from django.core.management.base import BaseCommand
from django.conf import settings

import os


class Command(BaseCommand):
    help = 'Closes the specified poll for voting'
    BaseCommand.requires_system_checks = []

    def handle(self, *args, **options):
        print('OS COMMAND:', os.environ.get('RUN_MAIN', None))

        if(os.path.exists(settings.KEYSFILE)):
            os.remove(settings.KEYSFILE)
        if(os.path.exists(settings.FILTERFILE)):
            os.remove(settings.FILTERFILE)
        print("FilterFiles purged")
        return
    
    def test(testfile):
        with open(testfile, 'wb') as tmp:
                tmp.write(b"test")
        tmp.close()
        os.remove(testfile)
        return