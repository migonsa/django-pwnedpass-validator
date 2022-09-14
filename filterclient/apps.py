from django.apps import AppConfig
from django.core.exceptions import ValidationError
from django.utils.translation import gettext as _
from django.conf import settings
from django.core.checks import Error, register
from django.core.management.color import color_style
from dbfilters import splitblockbloom, utils, ribbon128, binaryfuse8, xor8, xor16
from enum import Enum

import os, requests


filter = None
client_token = None


class ServerErrorCode(Enum):
    BAD_PASSWORD = 1
    BAD_ISSUER = 2
    BAD_TOKEN = 3
    EXPIRED_TOKEN = 4
    TOKEN_REQUIRED = 5
    TOKEN_UNNECESSARY = 6
    BAD_CREDENTIALS = 7
    BAD_FORMAT = 8

class ServerErrorMsg(Enum):
    BAD_PASSWORD = 'A password to check must be provided'
    BAD_ISSUER = 'Incorrect Issuer'
    BAD_TOKEN = 'Incorrect authorization token, try requesting a new one'
    EXPIRED_TOKEN = 'Token has expired, try requesting a new one'
    TOKEN_REQUIRED = 'A Bearer Authorization token must be provided'
    TOKEN_UNNECESSARY = 'Server is already open, no authentication is needed'
    BAD_CREDENTIALS = 'Unauthorized to use this service'
    BAD_FORMAT = 'Recieved incorrect format, must be JSON of type {username:user, password:pass}'

def dummy_false(password, hashed):
    return False

def dummy_true():
    return True

def clear_token():
    global client_token
    client_token = None
    return

def filter_parser():
    global filter
    #print('PARSER')
    #print(settings.FILTER)
    cons_args = [settings.KEYSFILE, settings.NKEYS]
    san_args = [settings.KEYSFILE, settings.NKEYS]
    save_args = [settings.FILTERFILE]
    load_args = [settings.FILTERFILE, settings.NKEYS]
    if (settings.FILTER  == 'ribbon128'):
        cons_args += [settings.RBYTES]
        load_args += [settings.RBYTES]
        if settings.OVERFACTOR is not None:
            cons_args += [settings.OVERFACTOR]
            load_args += [settings.OVERFACTOR]
        #print(cons_args)
        #print(load_args)
        filter = dict(cons = ribbon128.construct_filter,
                        cons_args = cons_args,
                        san = ribbon128.sanity_check,
                        san_args = san_args,
                        query = ribbon128.query_filter,
                        save = ribbon128.save_filter,
                        save_args = save_args,
                        load = ribbon128.load_filter,
                        load_args = load_args,
                        exist = ribbon128.exist_filter)
    elif (settings.FILTER  == 'splitblockbloom'):
        if settings.OVERFACTOR is not None:
            cons_args += [settings.OVERFACTOR]
            load_args += [settings.OVERFACTOR]
        filter = dict(cons = splitblockbloom.construct_filter,
                        cons_args = cons_args,
                        san = splitblockbloom.sanity_check,
                        san_args = san_args,
                        query = splitblockbloom.query_filter,
                        save = splitblockbloom.save_filter,
                        save_args = save_args,
                        load = splitblockbloom.load_filter,
                        load_args = load_args,
                        exist = splitblockbloom.exist_filter)
    elif (settings.FILTER  == 'binaryfuse8'):
        filter = dict(cons = binaryfuse8.construct_filter,
                        cons_args = cons_args,
                        san = binaryfuse8.sanity_check,
                        san_args = san_args,
                        query = binaryfuse8.query_filter,
                        save = binaryfuse8.save_filter,
                        save_args = save_args,
                        load = binaryfuse8.load_filter,
                        load_args = load_args,
                        exist = binaryfuse8.exist_filter)
    elif (settings.FILTER  == 'xor'):
        if (settings.RBYTES == 1):
            filter = dict(cons = xor8.construct_filter,
                        cons_args = cons_args,
                        san = xor8.sanity_check,
                        san_args = san_args,
                        query = xor8.query_filter,
                        save = xor8.save_filter,
                        save_args = save_args,
                        load = xor8.load_filter,
                        load_args = load_args,
                        exist = xor8.exist_filter)
        else:
            filter = dict(cons = xor16.construct_filter,
                        cons_args = cons_args,
                        san = xor16.sanity_check,
                        san_args = san_args,
                        query = xor16.query_filter,
                        save = xor16.save_filter,
                        save_args = save_args,
                        load = xor16.load_filter,
                        load_args = load_args,
                        exist = xor16.exist_filter)
    elif (settings.FILTER  == 'dummy'):
        filter = dict(dummy = True,
                    query = dummy_false,
                    exist = dummy_true)
    else:
        filter = None
    return
        

def add_filter():

    #print('OS APP:', os.environ.get('RUN_MAIN', None))
    filter_parser()
    if(filter == None or filter['exist']()):
        #print("FILTER ALREADY EXISTS")
        return
    #print("FILTER NOT IN MEM")
    if(os.path.exists(settings.FILTERFILE) and filter['load'](*filter['load_args'])):
        #print("FILTER LOADED")
        return
    #print("DJANGO1-BAD LOAD")
    if (os.path.exists(settings.KEYSFILE)):
        maxkeys = utils.calculate_keys(settings.KEYSFILE)
        if(settings.NKEYS > maxkeys):
            print("IGNORING NKEYS=%d SETTING... MAximum Keys in %s = %d" % (settings.NKEYS, settings.FILTERFILE, maxkeys))
        #print("\nINICIO: ", datetime.datetime.now(), "\n")
        if(filter['cons'](*filter['cons_args'])):
            pass 
            #print("\FIN CONS: ", datetime.datetime.now(), "\n")
            if (filter['san'](*filter['san_args'])):
                #print("\FIN SAN: ", datetime.datetime.now(), "\n")
                #print("CONSTRUCTED AND SANITIZED CORRECTLY")
                #print("SAVING1")
                filter['save'](*filter['save_args'])
                return
    #print("DJANGO1-BAD CONS")
    #raise ValidationError(_("FilterVal check failed"),code='password_no_number',)
    #common.synthetic(wd, nkeys)
    #splitblockbloom.construct_filter(wd, nkeys, 0.6)

def post_server():
    #print("QUERYING POST TO SERVER...")
    response = requests.post(settings.REMOTE_SERVER, json=settings.SERVER_CREDENTIALS)
    token = response.json().get('token')
    errorcode = response.json().get('errorcode')
    #print("TOKEN: %s  - ERRORCODE: %s" % (token, errorcode))
    if (token is not None or errorcode == ServerErrorCode.TOKEN_UNNECESSARY.value):
        global client_token
        client_token = token
        return True
    return False

def query_server(password):
    #print("QUERYING GET TO SERVER...")
    try:
        global client_token
        if (client_token is None):
            response = requests.get(settings.REMOTE_SERVER, params={'password':password})
        else:
            header = {'Authorization': 'Bearer %s' % client_token}
            response = requests.get(settings.REMOTE_SERVER, headers=header, params={'password':password})
        result = response.json().get('compromised')
        errorcode = response.json().get('errorcode')
        #print("RESULT: %s  - ERRORCODE: %s" % (result, errorcode))
        if (result is not None):
            return result
        else:
            if (errorcode == ServerErrorCode.TOKEN_REQUIRED.value or errorcode == ServerErrorCode.BAD_TOKEN.value):
                if(post_server()):
                    return query_server(password)
                else:
                    raise Exception('Unauthorized service')
            elif (errorcode == ServerErrorCode.BAD_PASSWORD.value):
                raise Exception('Password cannot be None or Empty')
            elif (errorcode == ServerErrorCode.BAD_ISSUER.value):
                raise Exception('Incorrect issuer')
            else:
                raise Exception('Unknown error')
    except Exception as e:
        pass
        #print(e)
    return True

def find_password(password, hashed = False):
    #print('OS VALIDATE:', os.environ.get('RUN_MAIN', None))
    if(settings.FILTER_MODE == 'REMOTE'):
        #print("HASHING...")
        hashed_pass = utils.sha1(password)
        #print("HASHED:", hashed_pass)
        return query_server(hashed_pass)
    if(filter is None):
        add_filter()
    return filter['query'](password, hashed)


class FilterclientConfig(AppConfig):
    default_auto_field = 'django.db.models.BigAutoField'
    name = 'filterclient'

    def ready(self):
        #print("READY CLIENT ", os.environ.get('RUN_MAIN', None))
        #print("FILTER ", settings.FILTER)
        if (os.environ.get('RUN_MAIN', None) == 'true'):
            if (settings.FILTER_MODE == 'LOCAL'):
                add_filter()
        return


@register()
def example_check(app_configs, **kwargs):
    errors = []
    #print('OS CHECK:', os.environ.get('RUN_MAIN', None))
    #print('TESTING:', os.environ.get('TESTING', None))
    #print("MODE:", settings.FILTER_MODE)
    if (os.environ.get('TESTING', None) != 'true'):
        if (settings.FILTER_MODE != 'LOCAL' and settings.FILTER_MODE != 'REMOTE'):
            errors.append(
                Error(
                    'Bad configuration of setting "FILTER_MODE',
                    hint='FILTER_MODE must be either <LOCAL> or <REMOTE>',
                    id='filterclient.E001',
                )
            )
        elif (settings.FILTER_MODE == 'LOCAL'):
            if (settings.FILTER not in settings.POSSIBLE_FILTERS):
                errors.append(
                    Error(
                        'Bad configuration of setting "FILTER',
                        hint='FILTER must be one of these: %s' % (settings.POSSIBLE_FILTERS),
                        id='filterclient.E002',
                    )
            )
            else:
                if(filter is None):
                    add_filter()
                elif(filter is not None and not filter['exist']()):
                    errors.append(
                        Error(
                            'Unknown error',
                            hint='Verify you have executed "preprocess" command after running django.',
                            id='filterclient.E003',
                        )
                    )
    return errors
