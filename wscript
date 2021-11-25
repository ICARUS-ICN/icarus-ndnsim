# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('icarus', ['mobility', 'ndnSIM'])
    module.source = [
        'model/circular-orbit.cc',
        'model/constellation.cc',
        'model/ground-sat-channel.cc',
        'model/ground-sat-success-distance.cc',
        'model/ground-sat-success-elevation.cc',
        'model/ground-sat-success-model.cc',
        'model/ground-sta-net-device.cc',
        'model/ground-sta-transport.cc',
        'model/sat2ground-net-device.cc',
        'model/satpos/planet.cc',
        'model/icarus-net-device.cc',
        'helper/constellation-helper.cc',
        'helper/icarus-helper.cc',
        'helper/isl-helper.cc',
        'utils/sat-address.cc',
        'model/sat2sat-channel.cc',
        'model/sat2sat-success-model.cc',
        'model/sat-net-device.cc',
        'model/mac-model.cc',
        'model/none-mac-model.cc',
        'model/aloha-mac-model.cc'
    ]

    module_test = bld.create_ns3_module_test_library('icarus')
    module_test.source = [
        'test/icarus-test-suite.cc',
        'test/icarus-address-test-suite.cc',
    ]

    headers = bld(features='ns3header')
    headers.module = 'icarus'
    headers.source = [
        'model/circular-orbit.h',
        'model/constellation.h',
        'model/sat2ground-net-device.h',
        'model/ground-sat-channel.h',
        'model/ground-sat-success-distance.h',
        'model/ground-sat-success-elevation.h',
        'model/ground-sat-success-model.h',
        'model/ground-sta-net-device.h',
        'model/ground-sta-transport.h',
        'model/icarus-net-device.h',
        'helper/constellation-helper.h',
        'helper/icarus-helper.h',
        'helper/isl-helper.h',
        'utils/sat-address.h',
        'model/sat2sat-channel.h',
        'model/sat2sat-success-model.h',
        'model/sat-net-device.h',
        'model/mac-model.h',
        'model/none-mac-model.h',
        'model/aloha-mac-model.h'
    ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()
