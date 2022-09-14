from django.urls import path
from django.views.decorators.csrf import csrf_exempt
from django.apps import apps
from django.conf import settings
from .views import FilterserverView

import os


if apps.is_installed("filterserver") or os.environ.get("TESTING") == 'true':
    urlpatterns = [
            path(settings.SERVER_URL, csrf_exempt(FilterserverView.as_view())),
    ]
else:
    urlpatterns = [
    ]
