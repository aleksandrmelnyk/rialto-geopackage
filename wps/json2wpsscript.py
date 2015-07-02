#!/usr/bin/env python

import io
import json
import os.path
import sys


# TODO: what if switch name is not a legal python variable, e.g. "--output-file"
# TODO: send datatypes to script correctly
# TODO: send enums to script correctly
# TODO: fix commas in lists
# TODO: in the script, call the tool via subprocess
# TODO: make file paths relative


g_datatypes = [
    'string',
    'int',
    'double',
    'position',
    'bbox',
    'file:raster',
    'file:elevation',
    'file:lidar',
    'file:geopackage',
    'file:text',
    'url:wms'
]


def parse_description(description):
    if not isinstance(description, basestring): raise TypeError('Description is not a string')


def parse_datatype(datatype, enums):
    if not isinstance(datatype, basestring): raise TypeError('Datatype is not a string')
    
    if datatype[0:5] == 'enum:':
        enum_type = datatype[5:]
        if enum_type not in enums:
            raise ValueError('Enum datatype not defined')
    else:
        if datatype not in g_datatypes:
            raise ValueError('Datatype not valid: %s' % datatype)


def parse_enum_values(enum_values):
    if len(enum_values) == 0:
        raise ValueError('Enum values not specified')
    for value in enum_values:
        if not isinstance(value, basestring): raise TypeError('Enum value is not a string')


def parse_enums(enums):
    if type(enums) is not dict: raise TypeError('Enums is not a map')
    for enum in enums:
        parse_enum_values(enums[enum])
    
    
def parse_param(data, enums):
    if type(data) is not dict: raise TypeError('Param is not a map')
    
    if 'datatype' not in data:
        raise ValueError('Param key "datatype" not found')

    parse_datatype(data['datatype'], enums)
    
    if 'description' in data:
        parse_description(data['description'])
    else:
        data['description'] = ''
    
    
def parse_params(data, enums):
    if type(data) is not dict: raise TypeError('Params is not a map')
    
    for k in data:
        parse_param(data[k], enums)


def parse_root(data):
    if type(data) is not dict: raise TypeError('Root is not a map')

    if 'name' not in data:
        raise ValueError('Root key "name" not found')

    if 'enums' in data:
        parse_enums(data['enums'])
    else:
        data['enums'] = {}

    if 'inputs' in data:
        parse_params(data['inputs'], data['enums'])
    else:
        data['inputs'] = []

    if 'outputs' in data:
        parse_params(data['outputs'], data['enums'])
    else:
        data['outputs'] = []

    if 'description' not in data:
        data['description'] = ''


def dump_enums(enums):
    print 'Enums:'
    for enum in enums:
        print '    %s:' % enum,
        for value in enums[enum]:
            print value,
        print


def dump_params(params, label):
    print '%s:' % label
    for param in params:
        print '    %s' % param
        print '        Datatype:', params[param]['datatype']
        print '        Description:', params[param]['description']


def dump(data):
    print 'Ttile:', data['name']
    print 'Description:', data['description']
    dump_enums(data['enums'])
    dump_params(data['inputs'], 'Inputs')
    dump_params(data['outputs'], 'Outputs')


def generate_script(data):
    print 'from geoserver.wps import process'
    print 'from com.vividsolutions.jts.geom import Geometry'
    print
    print '@process('
    print '    title=\'%s\',' % data['name']
    print '    description=\'%s\',' % data['description']
    
    print '    inputs={'
    for param in data['inputs']:
        attrs = data['inputs'][param]
        print '        \'%s\': (%s, \'%s\')' % (param, attrs['datatype'], attrs['description'])
    print '    }'
    
    print '    outputs={'
    for param in data['outputs']:
        attrs = data['outputs'][param]
        print '        %s: (%s, \'%s\')' % (param, attrs['datatype'], attrs['description'])
    print '    }'
    print

    print 'def run(',
    for param in data['inputs']:
        print param, ', ',
    print '):'
    print '    return FUNC(...)'


def main(jsonfiles):

    for jsonfile in jsonfiles:

        with io.open(jsonfile, 'r') as infile:
            print '============ %s ============' % os.path.basename(jsonfile)
            print '------------ json ------------'
            data = json.load(infile)
            parse_root(data)
            print data

            print '------------ dump ------------'
            dump(data)

            pyfile = os.path.splitext(jsonfile)[0] + ".py"
            print '------------ python ------------'
            generate_script(data)


if len(sys.argv) == 1:
    print 'Usage: $ %s [file1.json file2.json ...]' % os.path.basename(sys.argv[0])
    exit(1)

main(sys.argv[1:])
