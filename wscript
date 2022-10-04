# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

def configure(conf):
    conf.check_cfg(package='gsl', uselib_store='GSL', args='--cflags --libs', mandatory=True)

def build(bld):
    module = bld.create_ns3_module('icarus', ['mobility', 'ndnSIM'])
    module.source = [
        'helper/constellation-helper.cc',
        'helper/icarus-helper.cc',
        'helper/isl-helper.cc',
        'helper/lora-helper.cc',
        'helper/poisson-helper.cc',
        'model/circular-orbit.cc',
        'model/constellation.cc',
        'model/ground-node-sat-tracker.cc',
        'model/ground-sat-channel.cc',
        'model/ground-sat-success-distance.cc',
        'model/ground-sat-success-elevation.cc',
        'model/ground-sat-success-model.cc',
        'model/ground-sta-net-device.cc',
        'model/icarus-net-device.cc',
        'model/mac/aloha-mac-model.cc',
        'model/mac/crdsa-mac-model.cc',
        'model/mac/mac-model.cc',
        'model/mac/none-mac-model.cc',
        'model/ndn/ground-sta-transport.cc',
        'model/ndn/sat2ground-transport.cc',
        'model/orbit/circular-orbit-impl.cc',
        'model/orbit/satpos/planet.cc',
        'model/orbit/search/distancesolver.cc',
        'model/sat2ground-net-device.cc',
        'model/sat2sat-channel.cc',
        'model/sat2sat-success-model.cc',
        'model/sat-net-device.cc',
        'utils/sat-address.cc',
        'fw/geotag-strategy.cpp'
    ]

    module_test = bld.create_ns3_module_test_library('icarus')
    module_test.source = [
        'test/icarus-address-test-suite.cc',
        'test/icarus-mac-model-test-suite.cc',
        'test/icarus-test-suite.cc',
    ]

    headers = bld(features='ns3header')
    headers.module = 'icarus'
    headers.source = [
        'helper/constellation-helper.h',
        'helper/icarus-helper.h',
        'helper/isl-helper.h',
        'helper/lora-helper.h',
        'helper/poisson-helper.h',
        'model/circular-orbit.h',
        'model/constellation.h',
        'model/ground-node-sat-tracker.h',
        'model/ground-sat-channel.h',
        'model/ground-sat-success-distance.h',
        'model/ground-sat-success-elevation.h',
        'model/ground-sat-success-model.h',
        'model/ground-sta-net-device.h',
        'model/icarus-net-device.h',
        'model/mac/aloha-mac-model.h',
        'model/mac/crdsa-mac-model.h',
        'model/mac/mac-model.h',
        'model/mac/none-mac-model.h',
        'model/ndn/ground-sta-transport.h',
        'model/ndn/sat2ground-transport.h',
        'model/sat2ground-net-device.h',
        'model/sat2sat-channel.h',
        'model/sat2sat-success-model.h',
        'model/sat-net-device.h',
        'utils/sat-address.h',
        'fw/geotag-strategy.hpp'
    ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()
