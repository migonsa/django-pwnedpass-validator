# Miguel González Saiz
## Django PWNED Passwords Validator

django-pwnedpass-validator is a Django password validator that checks in an offline mode if a password has been involved in a major security breach before.


## Documentation

TBD

## Requirements

* Django 4 [4.0, 4.1]
* Python 3 [3.8, 3.9, 3.10]

## Quickstart


Install django-pwnedpass-validator:

    pip install django-pwnedpass-validator

Add it to your `INSTALLED_APPS`:


    INSTALLED_APPS = (
        ...
        'filterclient',
        'filterserver',
        ...
    )

Add django-pwnedpass-validator's FilterValidator:

    AUTH_PASSWORD_VALIDATORS = [
        ...
        {
            'NAME': 'filterclient.validators.FilterValidator'
        }
    ]


## Features

This password validator is made with AMQ data structures formed from the file of compromised passwords provided by the website [haveibeenpwned](https://haveibeenpwned.com/Passwords) in zip format. With this file that must be previously downloaded, an in-memory filter is created that works as a Django validator, that is to say, it returns a ValidationError if a password is compromised.


## Settings

TBD


## Running Tests
Tests are run with "test" Django's command:

    source <YOURVIRTUALENV>/bin/activate
    (myenv) $ python manage.py test filterclient filterserver

## License
MIT

**Free Software, Hell Yeah!**
