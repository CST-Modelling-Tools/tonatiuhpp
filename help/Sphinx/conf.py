# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))


# -- Project information -----------------------------------------------------

project = 'Tonatiuh++'
copyright = '2026, Tonatiuh++ contributors'
author = 'Tonatiuh++ contributors'

# The full version, including alpha/beta/rc tags
release = ''


# -- General configuration ---------------------------------------------------

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'alabaster'
#html_theme = 'nature'
#html_theme = 'bootstrap'
html_title = 'Tonatiuh++ Documentation'
html_short_title = 'Tonatiuh++'
html_theme_options = {
    'font_family': 'Arial',
    'font_size' : '8',
    #'show_powered_by' : 'false',
    'footer_text' : '#FFFFFF'
}
#html_theme = 'sphinx_rtd_theme' # too big
html_logo = "../../source/application/images/about/SplashScreen.png"
html_favicon = "../../source/application/images/about/Tonatiuh.ico"


# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = []
