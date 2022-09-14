from django.conf import settings
from django.core.management.commands import test

import os


class Command(test.Command):
    def handle(self, *args, **kwargs):
        if (kwargs['no_color']):
            os.environ["DJANGO_COLORS"] = "nocolor"
        super(Command, self).handle(*args, **kwargs)
        return