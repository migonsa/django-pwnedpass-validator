from django.apps import AppConfig
from django.apps import apps
from django.core.checks import Error, register
from django.core.management.utils import get_random_secret_key
from django.conf import settings

import os


secret = None

def random_secret():
    global secret
    secret = get_random_secret_key()

def get_secret():
    global secret
    return secret

class FilterserverConfig(AppConfig):
    default_auto_field = 'django.db.models.BigAutoField'
    name = 'filterserver'

    def ready(self):
        #print("READY SERVER ", os.environ.get('RUN_MAIN', None))
        if os.environ.get('RUN_MAIN', None) == 'true':
            random_secret()
        return



@register()
def example_check(app_configs, **kwargs):
	errors = []
	if (os.environ.get('TESTING', None) != 'true' and os.environ.get('RUN_MAIN', None) == 'true'):
		if(not apps.is_installed("filterclient")):
			errors.append(
			Error(
			'FilterClient missing',
			hint='App "filterclient" must be installed',
			id='filterserver.E001',
			)
			)
		elif(settings.FILTER_MODE != 'LOCAL'):
			errors.append(
			Error(
			'FilterClient compatibility error',
			hint='App "filterclient" must be in mode "LOCAL"',
			id='filterserver.E001',
			)
			)
		elif(secret == None):
			random_secret()
	return errors
