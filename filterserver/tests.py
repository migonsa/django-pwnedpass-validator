from django.test import TestCase, Client
from django.core.exceptions import ImproperlyConfigured
from django.test.utils import override_settings, modify_settings
from django.contrib.auth.models import User, Permission
from django.core.management.color import color_style
from django.conf import settings
from django.apps import apps
from filterclient.apps import add_filter, ServerErrorMsg, ServerErrorCode
from filterserver.apps import random_secret

import os, time, sys


def testing_mode(switch):
    if(os.environ.get('TESTING', None) != switch):
        os.environ["TESTING"] = switch

def check_installed(color, client=True, pwd=False, server=True):
        if (server and not apps.is_installed("filterserver")):
            raise ImproperlyConfigured(color.ERROR('APP "filterserver" must be installed in order to be tested'))
        if (client and not apps.is_installed("filterclient")):
            raise ImproperlyConfigured(color.ERROR('APP "filterserver" needs "filterclient" installed in order to be tested'))
        return

@override_settings(FILTER='dummy')
@override_settings(FILTER_MODE='LOCAL')
class FilterServer(TestCase):

    superuser = None
    perm_user = None
    no_perm_user = None
    random_user = None
    get_request = None
    get_response = None
    url = None
    color = None

    def __init__(self, *args, **kwargs):
        super(TestCase, self).__init__(*args, **kwargs)
        testing_mode('true')
        return

    @classmethod
    def tearDownClass(cls):
        super().tearDownClass()
        testing_mode('false')
        sys.stdout.write(color.HTTP_INFO('\n\nFinished SERVER tests.\n'))
        return

    @classmethod
    def setUpTestData(cls):
        super().setUpTestData()
        add_filter()
        random_secret()
        global superuser
        global perm_user
        global no_perm_user
        global random_user
        global get_request
        global get_response
        global url
        global color
        color = color_style()
        check_installed(color)
        User.objects.create_superuser(username='admin', email=None, password='admin')
        superuser = {'username':'admin', 'password':'admin'}
        User.objects.create_user(username='withperm', email=None, password='1234')
        permission = Permission.objects.get(codename='view_remoterequest')
        User.objects.get(username='withperm').user_permissions.add(permission)
        perm_user = {'username':'withperm', 'password':'1234'}
        User.objects.create_user(username='withoutperm', email=None, password='4321')
        no_perm_user = {'username':'withoutperm', 'password':'4321'}
        random_user = {'username':'random', 'password':'random'}
        get_request = {'password':'random_password_request'}
        get_response = {'password':'random_password_request', 'compromised': False}
        url = '/' + settings.SERVER_URL
        sys.stdout.write(color.HTTP_INFO('\nPerforming SERVER tests:\n'))
        #print("AUTH SUPERUSER: ", authenticate(username='admin', password='admin'))
        #print("REQ_PERM: ", authenticate(username='admin', password='admin').has_perm('filterserver.view_remoterequest'))
        #print("AUTH PERMUSER: ", authenticate(username='withperm', password='1234'))
        #print("REQ_PERM: ", authenticate(username='withperm', password='1234').has_perm('filterserver.view_remoterequest'))
        #print("AUTH NOPERM: ", authenticate(username='withoutperm', password='4321'))
        #print("REQ_PERM: ", authenticate(username='withoutperm', password='4321').has_perm('filterserver.view_remoterequest'))
        return

    @override_settings(OPEN_SERVER=True)
    def test_1(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting openserver post() 1...'))
        c = Client()
        response = c.post(url, superuser, content_type='application/json')
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.TOKEN_UNNECESSARY.value, 
                                        'errormsg': ServerErrorMsg.TOKEN_UNNECESSARY.value})

    @override_settings(OPEN_SERVER=True)
    def test_2(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting openserver post() 2...'))
        c = Client()
        response = c.post(url, perm_user, content_type='application/json')
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.TOKEN_UNNECESSARY.value, 
                                        'errormsg': ServerErrorMsg.TOKEN_UNNECESSARY.value})

    @override_settings(OPEN_SERVER=True)
    def test_3(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting openserver post() 3...'))
        c = Client()
        response = c.post(url, no_perm_user, content_type='application/json')
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.TOKEN_UNNECESSARY.value, 
                                        'errormsg': ServerErrorMsg.TOKEN_UNNECESSARY.value})

    @override_settings(OPEN_SERVER=True)
    def test_4(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting openserver post() 4...'))
        c = Client()
        response = c.post(url, random_user, content_type='application/json')
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.TOKEN_UNNECESSARY.value, 
                                        'errormsg': ServerErrorMsg.TOKEN_UNNECESSARY.value})

    @override_settings(OPEN_SERVER=True)
    def test_5(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting openserver post() 5...'))
        c = Client()
        response = c.post(url, {'whatever':'whatever'}, content_type='application/json')
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.TOKEN_UNNECESSARY.value, 
                                        'errormsg': ServerErrorMsg.TOKEN_UNNECESSARY.value})

    @override_settings(OPEN_SERVER=True)
    def test_6(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting openserver get() 1...'))
        c = Client()
        response = c.get(url, {'whatever':'whatever'}, content_type='application/json')
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.BAD_PASSWORD.value, 
                                        'errormsg': ServerErrorMsg.BAD_PASSWORD.value})

    @override_settings(OPEN_SERVER=True)
    def test_7(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting openserver get() 2...'))
        c = Client()
        response = c.get(url, {'password':''}, content_type='application/json')
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.BAD_PASSWORD.value, 
                                        'errormsg': ServerErrorMsg.BAD_PASSWORD.value})

    @override_settings(OPEN_SERVER=True)
    def test_8(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting openserver get() 3...'))
        c = Client()
        response = c.get(url, get_request, content_type='application/json')
        self.assertDictEqual(response.json(), get_response)


    @override_settings(OPEN_SERVER=False)
    def test_9(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting direct get() 1...'))
        c = Client()
        response = c.get(url, get_request, content_type='application/json')
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.TOKEN_REQUIRED.value, 
                                        'errormsg': ServerErrorMsg.TOKEN_REQUIRED.value})

    @override_settings(OPEN_SERVER=False)
    def test_10(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting direct get() 2...'))
        c = Client()
        response = c.get(url, get_request, content_type='application/json', **{'HTTP_AUTHORIZATION':'Bearer QVBJLUtFWTpnZW5lcmF0ZXNvbWVzZWN1cmVrZXlmb3J0aGlz'})
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.BAD_TOKEN.value, 
                                        'errormsg': ServerErrorMsg.BAD_TOKEN.value})

    @override_settings(OPEN_SERVER=False)
    def test_11(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting direct get() 3...'))
        c = Client()
        response = c.get(url, get_request, content_type='application/json', **{'HTTP_AUTHORIZATION':'bearer QVBJLUtFWTpnZW5lcmF0ZXNvbWVzZWN1cmVrZXlmb3J0aGlz'})
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.BAD_TOKEN.value, 
                                        'errormsg': ServerErrorMsg.BAD_TOKEN.value})

    @override_settings(OPEN_SERVER=False)
    def test_12(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting direct get() 4...'))
        c = Client()
        response = c.get(url, get_request, content_type='application/json', **{'HTTP_AUTHORIZATION':'QVBJLUtFWTpnZW5lcmF0ZXNvbWVzZWN1cmVrZXlmb3J0aGlz'})
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.BAD_TOKEN.value, 
                                        'errormsg': ServerErrorMsg.BAD_TOKEN.value})

    @override_settings(OPEN_SERVER=False)
    def test_13(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting direct get() 5...'))
        c = Client()
        response = c.get(url, get_request, content_type='application/json', **{'HTTP_AUTHORIZATION':''})
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.BAD_TOKEN.value, 
                                        'errormsg': ServerErrorMsg.BAD_TOKEN.value})
    
    @override_settings(OPEN_SERVER=False)
    def test_14(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting normal flow 1...'))
        c = Client()
        response = c.post(url, superuser, content_type='application/json')
        token = response.json().get('token')
        self.assertEqual(response.json().get('response'), 'authenticated')
        self.assertTrue(token != None)
        response = c.get(url, get_request, content_type='application/json', **{'HTTP_AUTHORIZATION': token})
        self.assertDictEqual(response.json(), get_response)

    @override_settings(OPEN_SERVER=False)
    def test_15(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting normal flow 2...'))
        c = Client()
        response = c.post(url, perm_user, content_type='application/json')
        token = response.json().get('token')
        self.assertEqual(response.json().get('response'), 'authenticated')
        self.assertTrue(token != None)
        response = c.get(url, get_request, content_type='application/json', **{'HTTP_AUTHORIZATION': 'Bearer ' + token})
        self.assertDictEqual(response.json(), get_response)

    @override_settings(OPEN_SERVER=False)
    def test_16(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting normal flow 3...'))
        c = Client()
        response = c.post(url, no_perm_user, content_type='application/json')
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.BAD_CREDENTIALS.value, 
                                        'errormsg': ServerErrorMsg.BAD_CREDENTIALS.value})

    @override_settings(OPEN_SERVER=False)
    def test_17(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting normal flow 4...'))
        c = Client()
        response = c.post(url, random_user, content_type='application/json')
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.BAD_CREDENTIALS.value, 
                                        'errormsg': ServerErrorMsg.BAD_CREDENTIALS.value})

    @override_settings(OPEN_SERVER=False)
    def test_18(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting normal flow 5...'))
        c = Client()
        response = c.post(url, {'whatever':'whatever'})
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.BAD_FORMAT.value, 
                                        'errormsg': ServerErrorMsg.BAD_FORMAT.value})

    @override_settings(OPEN_SERVER=False)
    def test_19(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting special 1...'))
        c = Client()
        with self.settings(TOKEN_DURATION=0):
            response = c.post(url, perm_user, content_type='application/json')
            token = response.json().get('token')
            self.assertEqual(response.json().get('response'), 'authenticated')
            self.assertTrue(token != None)
        time.sleep(1)
        response = c.get(url, get_request, content_type='application/json', **{'HTTP_AUTHORIZATION': 'Bearer ' + token})
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.EXPIRED_TOKEN.value, 
                                        'errormsg': ServerErrorMsg.EXPIRED_TOKEN.value})

    @override_settings(OPEN_SERVER=False)
    def test_20(self):
        sys.stdout.write(color.HTTP_INFO('\nTesting special 2...'))
        c = Client()
        with self.settings(TOKEN_ISSUER='random'):
            response = c.post(url, perm_user, content_type='application/json')
            token = response.json().get('token')
            self.assertEqual(response.json().get('response'), 'authenticated')
            self.assertTrue(token != None)
        response = c.get(url, get_request, content_type='application/json', **{'HTTP_AUTHORIZATION': 'Bearer ' + token})
        self.assertDictEqual(response.json(), {'errorcode': ServerErrorCode.BAD_ISSUER.value, 
                                        'errormsg': ServerErrorMsg.BAD_ISSUER.value})
