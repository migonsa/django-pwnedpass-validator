======================
Django PWNED Passwords Validator
======================

.. image:: https://badge.fury.io/py/django-pwned-passwords.svg
    :target: https://badge.fury.io/py/django-pwned-passwords

.. image:: https://travis-ci.org/jamiecounsell/django-pwned-passwords.svg?branch=master
    :target: https://travis-ci.org/jamiecounsell/django-pwned-passwords

.. image:: https://codecov.io/gh/jamiecounsell/django-pwned-passwords/branch/master/graph/badge.svg
    :target: https://codecov.io/gh/jamiecounsell/django-pwned-passwords

django-pwnedpass-validator is a Django password validator that checks in an offline mode if a password has been involved in a major security breach before.

**Note: This app currently sends a portion of a user's hashed password to a third party. Before using this application, you should understand how that impacts you.**

Documentation
-------------

The full documentation is at https://django-pwned-passwords.readthedocs.io.

Requirements
------------

* Django 4 [4.0, 4.1]
* Python 3 [3.8, 3.9, 3.10]

Quickstart
----------

Install django-pwnedpass-validator::

    pip install django-pwnedpass-validator

Add it to your `INSTALLED_APPS`:

..

    INSTALLED_APPS = (
        ...
        'filterclient',
        'filterserver',
        ...
    )

Add django-pwnedpass-validator's FilterValidator:

..

    AUTH_PASSWORD_VALIDATORS = [
        ...
        {
            'NAME': 'filterclient.validators.FilterValidator'
        }
    ]


Features
--------

This password validator returns a ValidationError if the PWNED Passwords API
detects the password in its data set. Note that the API is heavily rate-limited,
so there is a timeout (:code:`PWNED_VALIDATOR_TIMEOUT`).

If :code:`PWNED_VALIDATOR_FAIL_SAFE` is True, anything besides an API-identified bad password
will pass, including a timeout. If :code:`PWNED_VALIDATOR_FAIL_SAFE` is False, anything
besides a good password will fail and raise a ValidationError.


Settings
--------

+-------------------------------------------+---------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------+
| Setting                                   | Description                                                                                                         | Default                                                                                                                          |
+-------------------------------------------+---------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------+
| :code:`PWNED_VALIDATOR_TIMEOUT`           | The timeout in seconds. The validator will not wait longer than this for a response from the API.                   | :code:`2`                                                                                                                        |
+-------------------------------------------+---------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------+
| :code:`PWNED_VALIDATOR_FAIL_SAFE`         | If the API fails to get a valid response, should we fail safe and allow the password through?                       | :code:`True`                                                                                                                     |
+-------------------------------------------+---------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------+
| :code:`PWNED_VALIDATOR_URL`               | The URL for the API in a string format.                                                                             | :code:`https://haveibeenpwned.com/api/v2/pwnedpassword/{short_hash}`                                                             |
+-------------------------------------------+---------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------+
| :code:`PWNED_VALIDATOR_ERROR`             | The error message for an invalid password.                                                                          | :code:`"Your password was determined to have been involved in a major security breach."`                                         |
+-------------------------------------------+---------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------+
| :code:`PWNED_VALIDATOR_ERROR_FAIL`        | The error message when the API fails. Note: this will only display if :code:`PWNED_VALIDATOR_FAIL_SAFE` is `False`. | :code:`"We could not validate the safety of this password. This does not mean the password is invalid. Please try again later."` |
+-------------------------------------------+---------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------+
| :code:`PWNED_VALIDATOR_HELP_TEXT`         | The help text for this password validator.                                                                          | :code:`"Your password must not have been detected in a major security breach."`                                                  |
+-------------------------------------------+---------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------+
| :code:`PWNED_VALIDATOR_MINIMUM_BREACHES`  | The minimum number of breaches needed to raise an error                                                             | :code:`1`                                                                                                                        |
+-------------------------------------------+---------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------+


## Running Tests

    source <YOURVIRTUALENV>/bin/activate
    (myenv) $ pip install tox
    (myenv) $ tox

## License
MIT

**Free Software, Hell Yeah!**
