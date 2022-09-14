from django.test import TestCase, LiveServerTestCase
from django.core.exceptions import ImproperlyConfigured
from django.contrib.auth.models import User
from django.test.utils import override_settings, modify_settings
from django.core.management.utils import get_random_secret_key
from django.core.management.color import color_style
from django.conf import settings
from django.apps import apps
from filterclient.management.commands import purge, preprocess
from dbfilters import splitblockbloom, utils, ribbon128, binaryfuse8, xor8, xor16
from filterclient.apps import clear_token, post_server, query_server
from filterserver.apps import random_secret

import os, glob, filecmp, hashlib, sys


def testing_mode(switch):
    if (os.environ.get('TESTING', None) != switch):
        os.environ["TESTING"] = switch

def check_installed(color, client=True, server=False):
        if (client and not apps.is_installed("filterclient")):
            raise ImproperlyConfigured(color.ERROR('APP "filterclient" must be installed in order to be tested'))
        if (server and not apps.is_installed("filterserver")):
            raise ImproperlyConfigured(color.ERROR('APP "filterclient" needs "filterserver" installed in order to test remote client'))
        if (not os.path.exists(settings.PWDFILE)):
            raise ImproperlyConfigured(color.ERROR('Setting "PWDFILE" must be defined with a correct existing file'))
        if (not os.path.exists(settings.TESTING_DIR)):
            raise ImproperlyConfigured(color.ERROR('Setting "TESTING_DIR" must be defined with a correct existing directory'))
        
        return


class FilterLocalClient(TestCase):

    testing_nkeys = None
    testing_keysfile = None
    testing_filterfile = None
    color = None

    def __init__(self, *args, **kwargs):
        super(TestCase, self).__init__(*args, **kwargs)
        testing_mode('true')
        return
        
    @classmethod
    def tearDownClass(cls):
        super().tearDownClass()
        fileList = glob.glob(os.path.join(settings.TESTING_DIR, "*"))
        for i in fileList: os.remove(i)
        testing_mode('false')
        sys.stdout.write(color.HTTP_INFO('\n\nFinished LOCAL CLIENT tests.\n'))
        return

    @classmethod
    def setUpTestData(cls):
        super().setUpTestData()
        testing_mode('true')
        global testing_nkeys
        global testing_keysfile
        global testing_filterfile
        global color
        testing_nkeys = 100000
        testing_keysfile = settings.TESTING_DIR + "keystest.bin"
        testing_filterfile = settings.TESTING_DIR + "filtertest.bin"
        color = color_style()
        check_installed(color)
        sys.stdout.write(color.HTTP_INFO('\nPerforming LOCAL CLIENT tests:\n'))
        utils.synthetic(testing_keysfile, testing_nkeys)
        if (os.path.getsize(testing_keysfile) != testing_nkeys*20+21):
            raise Exception('mal')
        return


    def test_purge(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting command "purge"...'))
        purge_test_file = "tmp.bin"
        success = False
        purge.Command.test(os.path.join(settings.TESTING_DIR ,purge_test_file))
        if (not os.path.exists(os.path.join(settings.TESTING_DIR ,purge_test_file))):
            success = True
        self.assertTrue(success, color.ERROR("PURGE COMMAND FAILED TEST"))
        return


    def test_preprocess(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting command "preprocess"...'))
        pwdfile = "pwd.txt"
        prepfile = "keysprep.bin"
        with open(os.path.join(settings.TESTING_DIR, pwdfile), 'w') as pwd:
            with open(testing_keysfile, 'rb') as keys:
                keys.seek(21)
                while True:
                    data = keys.read(20)
                    if not data: break
                    for i in range(20):
                        pwd.write("%02x" % data[i])
                    pwd.write("\n")
            keys.close()
        pwd.close()
        preprocess.Command.test(os.path.join(settings.TESTING_DIR, pwdfile), os.path.join(settings.TESTING_DIR, prepfile), testing_nkeys)
        os.remove(os.path.join(settings.TESTING_DIR, pwdfile))
        self.assertTrue(filecmp.cmp(testing_keysfile, os.path.join(settings.TESTING_DIR, prepfile)), color.ERROR("PREPROCESS COMMAND FAILED TEST"))
        return

    def test_calculate_keys(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting c library "calculate_nkeys" function...'))
        self.assertEqual(utils.calculate_keys(testing_keysfile), testing_nkeys, color.HTTP_INFO("LIBRARY FUNCTION FAILED TEST"))
        return
    
    def test_sha1(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting c library "sha1" function...'))
        h = hashlib.new('sha1')
        hashstring = get_random_secret_key()
        h.update(hashstring.encode())
        self.assertEqual(utils.sha1(hashstring), h.hexdigest(), color.ERROR("LIBRARY FUNCTION FAILED TEST"))
        return

    
    def test_ribbon_1(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting ribbon128 filter with r=8...'))
        self.assertTrue(ribbon128.construct_filter(testing_keysfile, testing_nkeys), "Filter's construction failed.")
        self.assertTrue(ribbon128.exist_filter(), "Filter's construction failed.")
        self.assertTrue(ribbon128.sanity_check(testing_keysfile), "Filter's sanity check failed.")
        self.assertTrue(ribbon128.save_filter(testing_filterfile), "Filter's save failed.")
        ribbon128.destroy_filter()
        self.assertFalse(ribbon128.exist_filter(), "Filter's destruction failed.")
        self.assertTrue(ribbon128.load_filter(testing_filterfile), "Filter's load failed.")
        self.assertTrue(ribbon128.exist_filter(), "Filter's load failed.")
        self.assertTrue(ribbon128.sanity_check(testing_keysfile), "Filter's sanity check failed.")
        self.assertLessEqual(ribbon128.fp_filter(testing_nkeys)/testing_nkeys, (1/256)*1.30, color.ERROR("Filter's false positive ratio does not match theoretical value"))
        ribbon128.destroy_filter()
        return
    
    def test_ribbon_2(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting ribbon128 filter with r=16...'))
        self.assertTrue(ribbon128.construct_filter(testing_keysfile, testing_nkeys, 2), "Filter's construction failed.")
        self.assertTrue(ribbon128.exist_filter(), "Filter's construction failed.")
        self.assertTrue(ribbon128.sanity_check(testing_keysfile), "Filter's sanity check failed.")
        self.assertTrue(ribbon128.save_filter(testing_filterfile), "Filter's save failed.")
        ribbon128.destroy_filter()
        self.assertFalse(ribbon128.exist_filter(), "Filter's destruction failed.")
        self.assertTrue(ribbon128.load_filter(testing_filterfile, r=2), "Filter's load failed.")
        self.assertTrue(ribbon128.exist_filter(), "Filter's load failed.")
        self.assertTrue(ribbon128.sanity_check(testing_keysfile), "Filter's sanity check failed.")
        self.assertLessEqual(ribbon128.fp_filter(testing_nkeys*100)/(testing_nkeys*100), (1/65536)*1.30, color.ERROR("Filter's false positive ratio does not match theoretical value"))
        ribbon128.destroy_filter()
        return
    
    def test_bloom(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting split block bloom filter...'))
        self.assertTrue(splitblockbloom.construct_filter(testing_keysfile, testing_nkeys), "Filter's construction failed.")
        self.assertTrue(splitblockbloom.exist_filter(), "Filter's construction failed.")
        self.assertTrue(splitblockbloom.sanity_check(testing_keysfile), "Filter's sanity check failed.")
        self.assertTrue(splitblockbloom.save_filter(testing_filterfile), "Filter's save failed.")
        splitblockbloom.destroy_filter()
        self.assertFalse(splitblockbloom.exist_filter(), "Filter's destruction failed.")
        self.assertTrue(splitblockbloom.load_filter(testing_filterfile), "Filter's load failed.")
        self.assertTrue(splitblockbloom.exist_filter(), "Filter's load failed.")
        self.assertTrue(splitblockbloom.sanity_check(testing_keysfile), "Filter's sanity check failed.")
        self.assertLessEqual(splitblockbloom.fp_filter(testing_nkeys)/testing_nkeys, 0.01*1.30, color.ERROR("Filter's false positive ratio does not match theoretical value"))
        splitblockbloom.destroy_filter()
        return
    
    def test_binaryfuse(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting binary fuse filter with r=8...'))
        self.assertTrue(binaryfuse8.construct_filter(testing_keysfile, testing_nkeys), "Filter's construction failed.")
        self.assertTrue(binaryfuse8.exist_filter(), "Filter's construction failed.")
        self.assertTrue(binaryfuse8.sanity_check(testing_keysfile), "Filter's sanity check failed.")
        self.assertTrue(binaryfuse8.save_filter(testing_filterfile), "Filter's save failed.")
        binaryfuse8.destroy_filter()
        self.assertFalse(binaryfuse8.exist_filter(), "Filter's destruction failed.")
        self.assertTrue(binaryfuse8.load_filter(testing_filterfile), "Filter's load failed.")
        self.assertTrue(binaryfuse8.exist_filter(), "Filter's load failed.")
        self.assertTrue(binaryfuse8.sanity_check(testing_keysfile), "Filter's sanity check failed.")
        self.assertLessEqual(binaryfuse8.fp_filter(testing_nkeys)/testing_nkeys, (1/256)*1.30, color.ERROR("Filter's false positive ratio does not match theoretical value"))
        binaryfuse8.destroy_filter()
        return

    def test_xor_1(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting xor filter with r=8...'))
        self.assertTrue(xor8.construct_filter(testing_keysfile, testing_nkeys), "Filter's construction failed.")
        self.assertTrue(xor8.exist_filter(), "Filter's construction failed.")
        self.assertTrue(xor8.sanity_check(testing_keysfile), "Filter's sanity check failed.")
        self.assertTrue(xor8.save_filter(testing_filterfile), "Filter's save failed.")
        xor8.destroy_filter()
        self.assertFalse(xor8.exist_filter(), "Filter's destruction failed.")
        self.assertTrue(xor8.load_filter(testing_filterfile), "Filter's load failed.")
        self.assertTrue(xor8.exist_filter(), "Filter's load failed.")
        self.assertTrue(xor8.sanity_check(testing_keysfile), "Filter's sanity check failed.")
        self.assertLessEqual(xor8.fp_filter(testing_nkeys)/testing_nkeys, (1/256)*1.30, color.ERROR("Filter's false positive ratio does not match theoretical value"))
        xor8.destroy_filter()
        return

    def test_xor_2(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting xor filter with r=16...'))
        self.assertTrue(xor16.construct_filter(testing_keysfile, testing_nkeys), "Filter's construction failed.")
        self.assertTrue(xor16.exist_filter(), "Filter's construction failed.")
        self.assertTrue(xor16.sanity_check(testing_keysfile), "Filter's sanity check failed.")
        self.assertTrue(xor16.save_filter(testing_filterfile), "Filter's save failed.")
        xor16.destroy_filter()
        self.assertFalse(xor16.exist_filter(), "Filter's destruction failed.")
        self.assertTrue(xor16.load_filter(testing_filterfile), "Filter's load failed.")
        self.assertTrue(xor16.exist_filter(), "Filter's load failed.")
        self.assertTrue(xor16.sanity_check(testing_keysfile), "Filter's sanity check failed.")
        self.assertLessEqual(xor16.fp_filter(testing_nkeys*100)/(testing_nkeys*100), (1/65536)*1.30, color.ERROR("Filter's false positive ratio does not match theoretical value"))
        xor16.destroy_filter()
        return



@override_settings(FILTER='dummy')
@override_settings(FILTER_MODE='LOCAL')
@modify_settings(INSTALLED_APPS={'append': 'filterserver'})
class FilterRemoteClient(LiveServerTestCase):

    superuser = None
    perm_user = None
    no_perm_user = None
    random_user = None
    get_request = None
    get_response = None
    color = None

    def __init__(self, *args, **kwargs):
        super(LiveServerTestCase, self).__init__(*args, **kwargs)
        testing_mode('true')
        return

    @classmethod
    def tearDownClass(cls):
        super().tearDownClass()
        testing_mode('false')
        sys.stdout.write(color.HTTP_INFO('\n\nFinished REMOTE CLIENT tests.\n'))
        return

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        testing_mode('true')
        random_secret()
        global superuser
        global no_perm_user
        global random_user
        global color
        superuser = {'username':'admin', 'password':'admin'}
        no_perm_user = {'username':'withoutperm', 'password':'4321'}
        random_user = {'username':'random', 'password':'random'}
        color = color_style()
        check_installed(color, server=True)
        sys.stdout.write(color.HTTP_INFO('\nPerforming REMOTE CLIENT tests:\n'))
        return

    def aux(self, user):
        clear_token()
        User.objects.create_superuser(username='admin', email=None, password='admin')
        User.objects.create_user(username='withoutperm', email=None, password='4321')
        with self.settings(REMOTE_SERVER=self.live_server_url + '/' + settings.SERVER_URL):
            with self.settings(SERVER_CREDENTIALS=user):
                return (post_server(), query_server('whatever_pass'))


    @override_settings(OPEN_SERVER=True)
    def test_remote_client_1(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting superuser in open mode server...'))
        res = self.aux(superuser)
        self.assertTrue(res[0])
        self.assertFalse(res[1])

    
    @override_settings(OPEN_SERVER=True)
    def test_remote_client_2(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting unauthorized user in open mode server...'))
        res = self.aux(no_perm_user)
        self.assertTrue(res[0])
        self.assertFalse(res[1])

        
    @override_settings(OPEN_SERVER=True)
    def test_remote_client_3(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting random user in open mode server...'))
        res = self.aux(random_user)
        self.assertTrue(res[0])
        self.assertFalse(res[1])

    
    @override_settings(OPEN_SERVER=False)
    def test_remote_client_4(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting superuser in closed mode server...'))
        res = self.aux(superuser)
        self.assertTrue(res[0])
        self.assertFalse(res[1])

    @override_settings(OPEN_SERVER=False)
    def test_remote_client_5(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting unauthorized user in close mode server...'))
        res = self.aux(no_perm_user)
        self.assertFalse(res[0])
        self.assertTrue(res[1])


    @override_settings(OPEN_SERVER=False)
    def test_remote_client_6(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting random user in closed mode server...'))
        res = self.aux(random_user)
        self.assertFalse(res[0])
        self.assertTrue(res[1])
