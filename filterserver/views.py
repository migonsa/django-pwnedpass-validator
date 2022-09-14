from django.shortcuts import render
from django.http import JsonResponse
from django.views import View
from django.contrib.auth import authenticate
from django.conf import settings
from .apps import get_secret
from filterclient.apps import ServerErrorCode, ServerErrorMsg, find_password

import json, jwt, datetime


def check_password(request):
    pwd = request.GET.get('password')
    #print("PWD_REQUEST:",pwd)
    if(pwd is not None and pwd != ''):
        data = {
            'password': pwd,
            'compromised': find_password(pwd, True),
        }
    else:
        data = None
    return data


class FilterserverView(View):

    def get(self, request):
        errorcode = ServerErrorCode.BAD_TOKEN
        errormsg = ServerErrorMsg.BAD_TOKEN
        try:
            if(settings.OPEN_SERVER):
                data = check_password(request)
                if (data is None):
                    errorcode = ServerErrorCode.BAD_PASSWORD
                    errormsg = ServerErrorMsg.BAD_PASSWORD
                    raise Exception('A password to check must be provided')
            else:
                token = request.headers.get('Authorization')
                #print(token)
            
                if token is not None:
                    token = str.replace(str(token), 'Bearer ', '')
                    #print(token)
                    decoded_jwt = jwt.decode(token, get_secret(), algorithms=["HS256"], options={"require": ["exp", "iss"]})
                    if (decoded_jwt.get('iss') == settings.TOKEN_ISSUER):
                        data = check_password(request)
                        if (data is None):
                            errorcode = ServerErrorCode.BAD_PASSWORD
                            errormsg = ServerErrorMsg.BAD_PASSWORD
                            raise Exception('A password to check must be provided')
                    else:
                        errorcode = ServerErrorCode.BAD_ISSUER
                        errormsg = ServerErrorMsg.BAD_ISSUER
                        raise Exception('Incorrect Issuer')
                else:
                    errorcode = ServerErrorCode.TOKEN_REQUIRED
                    errormsg = ServerErrorMsg.TOKEN_REQUIRED
                    raise Exception('A Bearer Authorization token must be provided')
        except Exception as e:
            if str(e) == 'Signature has expired':
                errorcode = ServerErrorCode.EXPIRED_TOKEN
                errormsg = ServerErrorMsg.EXPIRED_TOKEN
            data = {
                    'errorcode': errorcode.value,
                    'errormsg': errormsg.value,
                }
            #print("INVALID GET: ", str(e))

        return JsonResponse(data)
    

    def post(self, request):
        errorcode = ServerErrorCode.BAD_FORMAT
        errormsg = ServerErrorMsg.BAD_FORMAT
        try:
            if (settings.OPEN_SERVER):
                errorcode = ServerErrorCode.TOKEN_UNNECESSARY
                errormsg = ServerErrorMsg.TOKEN_UNNECESSARY
                raise Exception('Server is already open, no authentication is needed')
            
            post_body = json.loads(request.body)
            username = post_body.get('username')
            password = post_body.get('password')
            #print("USER: ", username)
            #print("PASS: ", password)

            user = authenticate(username=username, password=password)
            #print("AUTH:", user)
            #print("REQ_PERM: ", user.has_perm('filterserver.view_remoterequest'))
            if user != None and user.has_perm('filterserver.view_remoterequest'):
                expire = datetime.datetime.now() + datetime.timedelta(hours=settings.TOKEN_DURATION)
                encoded_jwt = jwt.encode({"user": user.username, 
                                            "exp": datetime.datetime.now(tz=datetime.timezone.utc) + datetime.timedelta(hours=settings.TOKEN_DURATION),
                                            "iss": settings.TOKEN_ISSUER}, 
                                            get_secret(), algorithm="HS256")
                return JsonResponse({'response': 'authenticated', 'token': encoded_jwt})
            else:
                errorcode = ServerErrorCode.BAD_CREDENTIALS
                errormsg = ServerErrorMsg.BAD_CREDENTIALS
                raise Exception('Unauthorized to use this service')
        except Exception as e:
            data = {
                        'errorcode': errorcode.value,
                        'errormsg': errormsg.value,
                    }
            #print("INVALID POST: ", str(e))

        return JsonResponse(data)
