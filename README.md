# Miguel González Saiz
## Django PWNED Passwords Validator

django-pwnedpass-validator is a Django password validator that checks in an offline mode if a password has been involved in a major security breach before.


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
Within this module for Django there are two applications: one that acts as a client and one as a server. The client application is called *filterclient* and has two modes of operation: *LOCAL* and *REMOTE*. 
The local mode is designed so that a single instance of Django can have the filter in memory without relying on any other instance. It constructs a filter and uses it to validate passwords. Remote mode, however, is intended for organizations that have an infrastructure with multiple Django instances communicating with each other. This mode does not build any filters locally, but needs a server to query the passwords. That server has to be another Django instance running the *filterserver* server application, which will respond to all incoming requests with the result of the queries performed to the filter that it must have constructed in memory. Therefore, those Django instances destined to act as servers must have the *filterserver* and *filterclient* applications installed, and the latter must be in local mode to be able to construct the corresponding filter.


## Settings




## Running Tests
Tests are run with "test" Django's command:

    source <YOURVIRTUALENV>/bin/activate
    (myenv) $ python manage.py test filterclient filterserver

## License
MIT

**Free Software, Hell Yeah!**
