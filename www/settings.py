import os

RUNNING_APP_ENGINE_LOCAL_SERVER = os.environ.get('SERVER_SOFTWARE', 'Dev').startswith('Dev')

DEBUG = RUNNING_APP_ENGINE_LOCAL_SERVER # For now

APPEND_SLASH = True

INSTALLED_APPS = ['murphy']

MIDDLEWARE_CLASSES = [
    'django.middleware.common.CommonMiddleware',
    'django.middleware.http.ConditionalGetMiddleware',
    'murphy.middleware.AppEngineSecureSessionMiddleware',
]

DEBUG_SESSIONS = False # Set to True to get log lines about the contents of the session object

SITE_WIDE_USERNAME_AND_PASSWORD_URL_EXCEPTIONS = [ r'^/admin/taskqueue/.*$' ]

# NOTE davepeck:
#
# Add the following middleware classes
# if you want support for users in this application
# (I wrote these classes myself for another project)
#
# 'murphy.middleware.AppEngineGenericUserMiddleware',

ROOT_URLCONF = 'urls'

TEMPLATE_CONTEXT_PROCESSORS = [] 


# NOTE davepeck:
#
# (also add the following context processor if you want user support)
#
# 'murphy.context.appengine_user'

TEMPLATE_DEBUG = DEBUG

TEMPLATE_DIRS = [os.path.join(os.path.dirname(__file__), 'templates')]

TEMPLATE_LOADERS = ['django.template.loaders.filesystem.load_template_source']

SERIALIZATION_SECRET_KEY = '\xcfB\xf6\xb9\xc9\xd4\xfa\x07\x8atE\xdc\xec\xf9zaR\xa4\x13\x88'

LOGIN_URL = "/login/"

REDIRECT_FIELD_NAME = "redirect_url"

# only use local_settings.py if we're running debug server
if RUNNING_APP_ENGINE_LOCAL_SERVER:
    try:
        from local_settings import *
    except ImportError, exp:
        pass
