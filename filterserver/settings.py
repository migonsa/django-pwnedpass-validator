from django.conf import settings

TOKEN_DURATION = getattr(settings, 'TOKEN_DURATION', 72)
settings.TOKEN_DURATION = TOKEN_DURATION
TOKEN_ISSUER = getattr(settings, 'TOKEN_ISSUER', 'FilterServer')
settings.TOKEN_ISSUER = TOKEN_ISSUER
OPEN_SERVER = getattr(settings, 'OPEN_SERVER', True)
settings.OPEN_SERVER = OPEN_SERVER
SERVER_URL = getattr(settings, 'SERVER_URL', 'api/')
settings.SERVER_URL = SERVER_URL
