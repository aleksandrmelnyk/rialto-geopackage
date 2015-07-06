#!/usr/bin/env python

import io
import json
import os.path
import sys

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


def printf(s):
    sys.stdout.write(s)


def parse_description(description):
    if not isinstance(description, basestring): raise TypeError('Description is not a string')


def parse_datatype(datatype, enums):
    if not isinstance(datatype, basestring): raise TypeError('Datatype is not a string')
    
    if datatype.startswith('enum:'):
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




class Script:
    json = None

    def __init__(self, data):
        self.json = data
        
    # In the JSON the switch name might be "output-file",
    # but that's not a legal Python name, so do a s/-/__/
    #
    # To simplify life, we do not support switch names
    # already containing "__". 
    @staticmethod
    def canonicalize_option_name(name):
        if '__' in name:
            raise ValueError("invalid option name: %s" % name)
        new_name = name.replace('-', '__')
        return new_name

    @staticmethod
    def canonicalize_datatype_name(str):
        if str == 'string': return 'string'
        if str == 'int': return 'int'
        if str == 'double': return 'float'
        if str.startswith('enum:'): return 'string'
        if str.startswith('file:'): return 'string'
        if str.startswith('url:'): return 'string'
        raise ValueError('unknown datatype: %s' % str)

    def print_header_block(self):
        printf('from geoserver.wps import process\n')
        printf('from com.vividsolutions.jts.geom import Geometry\n')
        printf('from subprocess import Popen, PIPE\n')
        printf('\n')
        printf('@process(\n')
        printf('    title = \'%s\',\n' % self.json['name'])
        printf('    description = \'%s\',\n' % self.json['description'])
        
    def print_run_block(self):
        printf('def run(')
        sorted_list = self.json['outputs']
        for param in sorted_list:
            param_name = canonicalize_option_name(param)
            print param_name,
            if param_name != sorted_list[-1]:
                printf(', ')
        printf('):\n')

        printf('    cmd = "ls -l /"\n')
        printf('    p = Popen(cmd , shell=True, stdout=PIPE, stderr=PIPE\n')
        printf('    out, err = p.communicate()\n')
        printf('    print "Return code: ", p.returncode\n')
        printf('    print out.rstrip(), err.rstrip()\n')

    def print_param_list(self, io):
        printf('    %s = {\n' % io)
        sorted_list = sorted(self.json[io])
        for param in sorted_list:

            attrs = self.json[io][param]
            param_name = Script.canonicalize_option_name(param)
            datatype_name = Script.canonicalize_datatype_name(attrs['datatype'])
            printf('        \'%s\': (%s, \'%s\')' % (param_name, datatype_name, attrs['description']))
            if param != sorted_list[-1]:
                printf(',')
            printf('\n')
        printf('    }')

    def generate_script(self):
        self.print_header_block()

        self.print_param_list('inputs')
        printf(',\n')

        self.print_param_list('outputs')
        printf('\n')

        printf(')\n')
        printf('\n')

        self.print_run_block()

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
            script = Script(data)
            script.generate_script()


if len(sys.argv) == 1:
    print 'Usage: $ %s [file1.json file2.json ...]' % os.path.basename(sys.argv[0])
    exit(1)

main(sys.argv[1:])
