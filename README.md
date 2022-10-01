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

## Documentation

This work has been done for the thesis of the Master in Cybersecurity of the Universidad Carlos III de Madrid. All the work done has been documented in the following file: 
* [django-pwnedpass-validator.pdf](https://github.com/migonsa/django-pwnedpass-validator/blob/main/docs/django-pwnedpass-validator.pdf)

## Features

This password validator is made with AMQ data structures formed from the file of compromised passwords provided by the website [haveibeenpwned](https://haveibeenpwned.com/Passwords) in zip format. With this file that must be previously downloaded, an in-memory filter is created that works as a Django validator, that is to say, it returns a ValidationError if a password is compromised.
Within this module for Django there are two applications: one that acts as a client and one as a server. The client application is called *filterclient* and has two modes of operation: *LOCAL* and *REMOTE*. 
The local mode is designed so that a single instance of Django can have the filter in memory without relying on any other instance. It constructs a filter and uses it to validate passwords. Remote mode, however, is intended for organizations that have an infrastructure with multiple Django instances communicating with each other. This mode does not build any filters locally, but needs a server to query the passwords. That server has to be another Django instance running the *filterserver* server application, which will respond to all incoming requests with the result of the queries performed to the filter that it must have constructed in memory. Therefore, those Django instances destined to act as servers must have the *filterserver* and *filterclient* applications installed, and the latter must be in local mode to be able to construct the corresponding filter.


## Settings

| **Setting Name** | **Meaning**                                                                                                                                                    | **Possible Values**                                                           | **Default Value**                   | **Extra Info**                                                                                                                                                                                                                                                                                       |
|:----------------:|:--------------------------------------------------------------------------------------------------------------------------------------------------------------:|:-----------------------------------------------------------------------------:|:-----------------------------------:|:----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|
| *FILTER*         | Type of filter among the possible ones to be constructed and kept in memory                                                                                    | *ribbon128*<br />*xor*<br />*binaryfuse8*<br />*splitblockbloom*<br />*dummy* | *ribbon128*                         | There is a setting called *POSSIBLE_FILTERS* which contains all the possible filters to be constructed. The *dummy* filter is for testing purposes, because it always returns True.                                                                                                                  |
| *RBYTES*         | Number of bytes of the fingerprint (a.k.a. *r*). Only applicable to *ribbon128* and *xor* filters.                                                             | *1*<br />*2*                                                                  | *1*                                 | The larger *RBYTES* the fewer false positive rate (FPR) but, at the same time, the bigger the filter results and the more memory it needs.                                                                                                                                                           |
| *NKEYS*          | Number of keys to construct the filter with. *0* would mean all the keys in *KEYSFILE*.                                                                        | Whatever number.                                                              | *0*                                 | -                                                                                                                                                                                                                                                                                                    |
| *OVERFATOR*      | The resulting memory occupied bytes per key, divided by the ideal bytes per key (for an ideal filter if $*r*=8$, then the *OVERFACTOR* would be 1.             | Whatever number.                                                              | The optimized ones for each filter. | The higher the *OVERFACTOR* the lower the FPR to the theoretical minimum ($1/(2^r$) at the cost of memory occupancy.                                                                                                                                                                                 |
| *KEYSFILE*       | Path to file containing the preprocessed keys resulted from the execution of the *preprocess* command.                                                         | Custom to each user.                                                          | -                                   | Command *preprocess* must be executed before in order to get a *KEYSFILE*.                                                                                                                                                                                                                           |
| *FLTERFILE*      | Path to file containing the last constructed filter, so as to load into memory next time without having to be constructed again.                               | Custom to each user.                                                          | -                                   | At least one execution of Django's instance having installed *filterclient* application must be completed in order to get a valid *FILTERFILE* for next execution. Note that this file is only valid if the settings *FILTER*, *RBYTES*, *NKEYS* and *OVERFATOR* remain the same between executions. |
| *TESTING_DIR*    | Path to testing directory, used whenever the *filterclient* application is installed and wanted to be tested as indicated in the next section "Running Tests". | Custom to each user.                                                          | -                                   | -                                                                                                                                                                                                                                                                                                    |



## Running Tests
Tests are run with "test" Django's command:

    source <YOURVIRTUALENV>/bin/activate
    (myenv) $ python manage.py test filterclient filterserver

## License
MIT

**Free Software, Hell Yeah!**
