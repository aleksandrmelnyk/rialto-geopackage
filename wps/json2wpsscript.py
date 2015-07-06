#!/usr/bin/env python

import io
import json
import os.path
import sys


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



def fprintf(file, str):
    file.write(str)



class Parse:
    
    def parse_description(self, description):
        if not isinstance(description, basestring): raise TypeError('Description is not a string')


    def parse_datatype(self, datatype, enums):
        if not isinstance(datatype, basestring): raise TypeError('Datatype is not a string')
        
        if datatype.startswith('enum:'):
            enum_type = datatype[5:]
            if enum_type not in enums:
                raise ValueError('Enum datatype not defined')
        else:
            if datatype not in g_datatypes:
                raise ValueError('Datatype not valid: %s' % datatype)

    def parse_enum_values(self, enum_values):
        if len(enum_values) == 0:
            raise ValueError('Enum values not specified')
        for value in enum_values:
            if not isinstance(value, basestring): raise TypeError('Enum value is not a string')

    def parse_enums(self, enums):
        if type(enums) is not dict: raise TypeError('Enums is not a map')
        for enum in enums:
            self.parse_enum_values(enums[enum])

    def parse_param(self, data, enums):
        if type(data) is not dict: raise TypeError('Param is not a map')
        
        if 'datatype' not in data:
            raise ValueError('Param key "datatype" not found')

        self.parse_datatype(data['datatype'], enums)
        
        if 'description' in data:
            self.parse_description(data['description'])
        else:
            data['description'] = ''
        
    def parse_params(self, data, enums):
        if type(data) is not dict: raise TypeError('Params is not a map')
        
        for k in data:
            self.parse_param(data[k], enums)

    def parse_root(self, data):
        if type(data) is not dict: raise TypeError('Root is not a map')

        if 'name' not in data:
            raise ValueError('Root key "name" not found')

        if 'description' not in data:
            data['description'] = ''

        if 'enums' in data:
            self.parse_enums(data['enums'])
        else:
            data['enums'] = {}

        if 'inputs' not in data:
            raise ValueError('Root key "inputs" not found')
        self.parse_params(data['inputs'], data['enums'])

        if 'outputs' not in data:
            raise ValueError('Root key "outputs" not found')
        self.parse_params(data['outputs'], data['enums'])



class Dump:
    
    def dump_enums(self, enums):
        print 'Enums:'
        for enum in enums:
            print '    %s:' % enum,
            for value in enums[enum]:
                print value,
            print

    def dump_params(self, params, label):
        print '%s:' % label
        for param in params:
            attrs = params[param]
            print '    %s' % param
            print '        Datatype:', attrs['datatype']
            print '        Description:', attrs['description']

    def dump(self, data):
        print 'Ttile:', data['name']
        print 'Description:', data['description']
        self.dump_enums(data['enums'])
        self.dump_params(data['inputs'], 'Inputs')
        self.dump_params(data['outputs'], 'Outputs')



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
        if str == 'string': return 'str'
        if str == 'int': return 'int'
        if str == 'double': return 'float'
        if str == 'position': return 'str'
        if str.startswith('enum:'): return 'str'
        if str.startswith('file:'): return 'str'
        if str.startswith('url:'): return 'str'
        raise ValueError('unknown datatype: %s' % str)

    def print_header_block(self, f):
        fprintf(f, '#!/usr/bin/env python\n')
        fprintf(f, '\n')
        fprintf(f, 'from geoserver.wps import process\n')
        fprintf(f, 'from com.vividsolutions.jts.geom import Geometry\n')
        fprintf(f, 'import re\n')
        fprintf(f, 'from subprocess import Popen, PIPE\n')
        fprintf(f, '\n')
        fprintf(f, '@process(\n')
        fprintf(f, '    title = \'%s\',\n' % self.json['name'])
        fprintf(f, '    description = \'%s\',\n' % self.json['description'])
        
    def print_run_block(self, f):
        fprintf(f, 'def run(')
        if self.json['inputs']:
            params = self.json['inputs'].keys()
            params.sort()
            for param in params:
                param_name = Script.canonicalize_option_name(param)
                fprintf(f, param_name)
                if param_name != params[-1]:
                    fprintf(f, ', ')
        fprintf(f, '):\n')

        fprintf(f, '    cmd = "%s/%s.sh' % ("/Users/mgerlek/work/dev/rialto-geopackage/wps/tests", self.json['name']))
        if self.json['inputs']:
            params = self.json['inputs'].keys()
            params.sort()
            for param in params:
                param_name = Script.canonicalize_option_name(param)
                fprintf(f, " --%s %s" % (param, param_name))
        fprintf(f, '"\n')
        
        fprintf(f, """
    p = Popen(cmd , shell=True, stdout=PIPE, stderr=PIPE)
    out, err = p.communicate()
    print "Return code: ", p.returncode
    print out.rstrip(), err.rstrip()
    
    results = {"stdout":out, "stderr": err, "status": p.returncode}
    p = re.compile('^\[(\w+)\]\s*(.*)')
    for line in out.split("\\n"):
        ms = p.findall(line)
        for (k,v) in ms:
            results[k] = v
    
    return results
""")

    def print_inputs_list(self, f):
        fprintf(f, '    inputs = {\n')
        sorted_list = sorted(self.json['inputs'])
        for param in sorted_list:

            attrs = self.json['inputs'][param]
            param_name = Script.canonicalize_option_name(param)
            datatype_name = Script.canonicalize_datatype_name(attrs['datatype'])
            descr = '%s [%s]' % (attrs['description'], attrs['datatype'])
            fprintf(f, '        \'%s\': (%s, \'%s\')' % (param_name, datatype_name, descr))
            if param != sorted_list[-1]:
                fprintf(f, ',')
            fprintf(f, '\n')
        fprintf(f, '    }')

    def print_outputs_list(self, f):
        fprintf(f, """
    outputs = {
        'status': (int, 'the return code from the script [int]'),
        'stdout': (str, 'stdout from the script [string]'),
        'stderr': (str, 'stderr from the script [string]')""")

        sorted_list = sorted(self.json['outputs'])
        for param in sorted_list:

            fprintf(f, ',\n')
            attrs = self.json['outputs'][param]
            param_name = Script.canonicalize_option_name(param)
            datatype_name = Script.canonicalize_datatype_name(attrs['datatype'])
            descr = '%s [%s]' % (attrs['description'], attrs['datatype'])
            fprintf(f, '        \'%s\': (%s, \'%s\')' % (param_name, datatype_name, descr))

        fprintf(f, """
    }
""")
        fprintf(f, ')\n')

    def generate_script(self, filename):
        f = open(filename, "w")
        self.print_header_block(f)

        self.print_inputs_list(f)
        fprintf(f, ',\n')

        self.print_outputs_list(f)
        fprintf(f, '\n')

        self.print_run_block(f)
        f.close()



def main(jsonfile, pyfile):

    with io.open(jsonfile, 'r') as infile:
        print '============ %s ============' % os.path.basename(jsonfile)
        data = json.load(infile)
        parse = Parse()
        parse.parse_root(data)
        
        #print '------------ json ------------'
        #print data

        #print '------------ dump ------------'
        dump = Dump()
        #dump.dump(data)

        print '------------ python ------------'
        script = Script(data)
        script.generate_script(pyfile)


if len(sys.argv) != 3:
    print 'Usage: $ %s file.json file.py' % os.path.basename(sys.argv[0])
    exit(1)

main(sys.argv[1], sys.argv[2])
