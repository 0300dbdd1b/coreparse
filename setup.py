import os
from setuptools import setup, Extension
from Cython.Build import cythonize

# Get the absolute path to the root of your repository
# We assume libcoreparse.so is generated here by your Makefile
current_dir = os.path.abspath(os.path.dirname(__file__))

extensions = [
    Extension(
        "coreparse", 
        sources=["python/coreparse.pyx"],  # ONLY compile the Cython file now
        include_dirs=["src/include", "src"],
        # Tell the linker where to find libcoreparse.so
        library_dirs=[current_dir], 
        # Link against your library (-lcoreparse), leveldb, and stdc++
        libraries=["coreparse", "leveldb", "stdc++"], 
        extra_compile_args=["-O3"],
        # CRITICAL: Embed the directory path into the compiled Python module.
        # Without this, Python will throw a "libcoreparse.so not found" error 
        # at runtime unless you manually set LD_LIBRARY_PATH.
        extra_link_args=[f"-Wl,-rpath,{current_dir}"] 
    )
]

setup(
    name="coreparse",
    version="1.0.0",
    ext_modules=cythonize(extensions, language_level="3"),
    install_requires=["Cython"]
)
