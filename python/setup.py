from setuptools import setup, find_packages

setup(name = "pyrtcdcpp",
      version = "0.1",
      packages = find_packages(),
      setup_requires=["cffi>=1.10.0"],
      cffi_modules=["pyrtcdcpp_build.py:ffibuilder"],
      install_requires = ["cffi>=1.10.0"],
      description = "Python bindings for the librtcdcpp library",
      url = "http://github.com/hamon-in/librtcdcpp",
)
