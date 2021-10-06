# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('icarus', ['mobility', 'ndnSIM'])
    module.source = [
        'model/circular-orbit.cc',
        'model/satpos/planet.cc',
        'helper/icarus-helper.cc',
    ]

    module_test = bld.create_ns3_module_test_library('icarus')
    module_test.source = [
        'test/icarus-test-suite.cc',
    ]

    headers = bld(features='ns3header')
    headers.module = 'icarus'
    headers.source = [
        'model/circular-orbit.h',
        'helper/icarus-helper.h',
    ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()
