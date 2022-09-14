from django.core.management.base import BaseCommand
from dbfilters import preprocess
from django.conf import settings

import os


class Command(BaseCommand):
    help = 'Closes the specified poll for voting'
    BaseCommand.requires_system_checks = []

    def handle(self, *args, **options):
        print('OS COMMAND:', os.environ.get('RUN_MAIN', None))

        if(os.path.exists(settings.PWDFILE)):
            if(preprocess.preprocess_pwd_file(settings.PWDFILE, settings.KEYSFILE, settings.PREPKEYS, settings.CHECKPREP)):
                print('PREPROCESS DONE')
            else:
                print("DJANGO1-BAD PREPROCESS")
        else:
            print("PWD NOT FOUND")
        return
    
    def test(pwdfile, testfile, nkeys):
        if(os.path.exists(pwdfile)):
            preprocess.preprocess_pwd_file(pwdfile, testfile, nkeys, True)
        return
