from django.conf import settings

FILTER = getattr(settings, 'FILTER', 'ribbon128')
settings.FILTER = FILTER
POSSIBLE_FILTERS = getattr(settings, 'POSSIBLE_FILTERS', ['ribbon128', 'xor', 'binaryfuse8', 'splitblockbloom', 'dummy'])
settings.POSSIBLE_FILTERS = POSSIBLE_FILTERS
NKEYS = getattr(settings, 'NKEYS', 0)
settings.NKEYS = NKEYS
RBYTES = getattr(settings, 'RBYTES', 1)
settings.RBYTES = RBYTES
OVERFACTOR = getattr(settings, 'OVERFACTOR', None)
settings.OVERFACTOR = OVERFACTOR

PREPKEYS = getattr(settings, 'PREPKEYS', 1000000)
settings.PREPKEYS = PREPKEYS
CHECKPREP = getattr(settings, 'CHECKPREP', True)
settings.CHECKPREP = CHECKPREP

PWDFILE = getattr(settings, 'PWDFILE', '../../FilterPassword/pwd_full.txt')
settings.PWDFILE = PWDFILE
KEYSFILE = getattr(settings, 'KEYSFILE', 'filterclient/FilterFiles/keys.bin')
settings.KEYSFILE = KEYSFILE
FILTERFILE = getattr(settings, 'FILTERFILE', 'filterclient/FilterFiles/filter.bin')
settings.FILTERFILE = FILTERFILE

FILTER_MODE = getattr(settings, 'FILTER_MODE', 'LOCAL')
settings.FILTER_MODE = FILTER_MODE
REMOTE_SERVER = getattr(settings, 'REMOTE_SERVER', 'http://127.0.0.1:8000/reqfilter')
settings.REMOTE_SERVER = REMOTE_SERVER
SERVER_CREDENTIALS = getattr(settings, 'SERVER_CREDENTIALS', {'username':'admin', 'password':'admin'})
settings.SERVER_CREDENTIALS = SERVER_CREDENTIALS

TESTING_DIR = getattr(settings, 'TESTING_DIR', 'filterclient/TestingFiles/')
settings.TESTING_DIR = TESTING_DIR