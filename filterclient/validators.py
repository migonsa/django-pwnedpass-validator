import re

from django.core.exceptions import ValidationError
from django.utils.translation import gettext as _
from filterclient.apps import find_password


class FilterValidator(object):

    def validate(self, password, user=None):
        if find_password(password):
            raise ValidationError(
            _("FilterValidator check failed"),
            code='FilterValidator',
            )

    def get_help_text(self):
        return _("Your password's length must be equal or greater than 8 and cannot be compromised.")


