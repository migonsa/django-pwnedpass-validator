from os.path import dirname, abspath
from django.conf import settings
from filterclient.settings import *


'''def load_tests(loader, tests, pattern):
    from django.apps import apps
    if apps.is_installed("filterclient"):
        return loader.discover(start_dir=dirname(abspath(__file__)), pattern=pattern)'''