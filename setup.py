from setuptools.command.build_ext import build_ext
from setuptools import setup, Extension


class custom_build_ext(build_ext):
    def build_extensions(self):
        # Override the compiler executables. Importantly, this
        # removes the "default" compiler flags that would
        # otherwise get passed on to to the compiler, i.e.,
        # distutils.sysconfig.get_var("CFLAGS").
        self.compiler.set_executable("compiler_so", "gcc")
        self.compiler.set_executable("compiler_c", "gcc")
        self.compiler.set_executable("linker_so", "gcc")
        build_ext.build_extensions(self)

ext_modules = [
    Extension('dbfilters.utils', 
                sources = ['dbfilters/src/utils_wrapper.c'],
                extra_objects=["dbfilters/src/sha1-avx.S"],
                extra_compile_args = ["-fPIC", "-fno-strict-aliasing", "-mlzcnt", "-O3", "-mavx2", "-march=native"],
                extra_link_args=["-shared", "-Wl,-O3", "-Wl,-Bsymbolic-functions", "-lstdc++"]
                ),
    Extension('dbfilters.preprocess', 
               sources = ['dbfilters/src/preprocess_wrapper.c'],
               extra_objects=[],
               extra_compile_args = ["-fPIC", "-fno-strict-aliasing", "-mlzcnt", "-O3", "-mavx2", "-march=native"],
               extra_link_args=["-shared", "-Wl,-O3", "-Wl,-Bsymbolic-functions", "-lstdc++"]
               ),
    Extension('dbfilters.ribbon128', 
                sources = ['dbfilters/src/ribbon_wrapper.c'],
                extra_objects=["dbfilters/src/sha1-avx.S"],
                extra_compile_args = ["-fPIC", "-fno-strict-aliasing", "-mlzcnt", "-O3", "-mavx2", "-march=native"],
                extra_link_args=["-shared", "-Wl,-O3", "-Wl,-Bsymbolic-functions", "-lstdc++"]
                ),
    Extension('dbfilters.binaryfuse8', 
                sources = ['dbfilters/src/binaryfuse_wrapper.c'],
                extra_objects=["dbfilters/src/sha1-avx.S"],
                extra_compile_args = ["-fPIC", "-fno-strict-aliasing", "-mlzcnt", "-O3", "-mavx2", "-march=native"],
                extra_link_args=["-shared", "-Wl,-O3", "-Wl,-Bsymbolic-functions", "-lstdc++"]
                ),
    Extension('dbfilters.xor8', 
                sources = ['dbfilters/src/xor8_wrapper.c'],
                extra_objects=["dbfilters/src/sha1-avx.S"],
                extra_compile_args = ["-fPIC", "-fno-strict-aliasing", "-mlzcnt", "-O3", "-mavx2", "-march=native"],
                extra_link_args=["-shared", "-Wl,-O3", "-Wl,-Bsymbolic-functions", "-lstdc++"]
                ),
    Extension('dbfilters.xor16', 
                sources = ['dbfilters/src/xor16_wrapper.c'],
                extra_objects=["dbfilters/src/sha1-avx.S"],
                extra_compile_args = ["-fPIC", "-fno-strict-aliasing", "-mlzcnt", "-O3", "-mavx2", "-march=native"],
                extra_link_args=["-shared", "-Wl,-O3", "-Wl,-Bsymbolic-functions", "-lstdc++"]
                ),
    Extension('dbfilters.splitblockbloom', 
                sources = ['dbfilters/src/splitblockbloom_wrapper.c'],
                extra_objects=["dbfilters/src/sha1-avx.S"],
                extra_compile_args = ["-fPIC", "-fno-strict-aliasing", "-mlzcnt", "-O3", "-mavx2", "-march=native"],
                extra_link_args=["-shared", "-Wl,-O3", "-Wl,-Bsymbolic-functions", "-lstdc++"]
                )
]
setup(
    name = 'django-pwnedpass-validator',
    version = '0.0.1',
    description = 'DESCRIPTION...',
    license = 'MIT',
    author = 'Miguel González Saiz',
    author_email = '100346858@alumnos.uc3m.es',
    url = 'TBD.tbd',
    download_url = 'TBD.tbd',
    keywords = ['django', 'installable django app', 'validator', 'pwned passwords'],
    install_requires = [
    	'Django>=4.0'
    	'requests>=2.28'
    	'PyJWT>=2.4'
    ],
    python_requires = '>=3.8',
	setup_requires = ['setuptools'],
    ext_modules = ext_modules,
    packages = ['dbfilters', 'src', 'filterclient', 'filterserver', 'filterclient.management.commands', 'filterclient.migrations', 'filterserver.management.commands', 'filterserver.migrations'],
    package_dir={'src': 'dbfilters/src'},
    package_data={'src': ['*']},
    cmdclass = {"build_ext": custom_build_ext},
    classifiers = [
    	'Development Status :: 3 - Alpha',
		'Intended Audience :: Developers',
		'License :: OSI Approved :: MIT License',
		'Operating System :: Linux',
		'Programming Language :: Python :: 3 :: Only',
		'Programming Language :: Python :: Implementation :: CPython',
		'Topic :: Software Development :: Libraries :: Application Frameworks',
		'Topic :: Software Development :: Libraries :: Python Modules',
	]
)
