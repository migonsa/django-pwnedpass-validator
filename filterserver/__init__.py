from os.path import dirname, abspath
from django.conf import settings
from filterserver.settings import *


'''def load_tests(loader, tests, pattern):
    from django.apps import apps
    if apps.is_installed("filterserver"):
        return loader.discover(start_dir=dirname(abspath(__file__)), pattern=pattern)'''