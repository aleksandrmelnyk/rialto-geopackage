#!/usr/bin/env python

# Because I can't seem to get GeoServer's WPS/Python interface to accept
# any annotations for the input and output parameters, we're just going to
# pass the info we need at the end of the parameter's description string.
# Each param will have a description string formatted like this:
#
#     blah blah blah [TYPE] [ANNOTATION]
#
# The [TYPE] part is required, and TYPE will be one of the native python types.
# 
# The [ANNOTATION] part is optional. If present and TYPE is string, then the
# ANNOTATION will be a list of the enum values. If present and TYPE is int or
# double, then the ANNOTATION will be a list of the range values, i.e. min
# and max.

import io
import json
import os.path
import sys


g_datatypes = [
    'int',
    'double',
    'string',
    'string:raster_file',
    'string:elevation_file',
    'string:lidar_file',
    'string:geopackage_file',
    'string:text_file',
    'string:wms_url',
    'geo_pos_2d',
    'geo_box_2d'
]

def escapify(s):
    return s.replace("'", "\\'")

def check_is_string(obj, msg):
    if not obj: return
    if not isinstance(obj, basestring):
        raise TypeError(msg)

def check_is_instance(obj, typ, msg):
    if not obj: return
    if not isinstance(obj, typ):
        raise TypeError(msg)

def check_is_type(obj, typ, msg):
    if not obj: return
    if type(obj) != typ:
        raise TypeError(msg)

def check_contains(item, items, msg):
    if not item: return
    if item not in items:
        raise ValueError(msg)


class DataType:
    _text = None
    enum_values = None
    min = None
    max = None
    range_type = None
    enum_type = None
    
    def __init__(self, text, enums, ranges):
        check_is_string(text, 'datatype is not a string')

        self._text = text 
        
        if self._text.startswith('enum:'):
            self.enum_type = self._text[5:]
            check_contains(self.enum_type, enums, 'enum datatype not defined')
            self.enum_values = list(enums[self.enum_type])
            return
            
        if self._text.startswith('int:'):
            self.range_type = self._text[4:]
            check_contains(self.range_type, ranges, 'range datatype not defined: %s' % self.range_type)
            self.min = ranges[self.range_type][0]
            self.max = ranges[self.range_type][1]
            return
            
        if self._text.startswith('double:'):
            range_type = self._text[7:]
            check_contains(self.range_type, ranges, 'range datatype not defined: %s' % self.range_type)
            self.min = ranges[range_type][0]
            self.max = ranges[range_type][1]
            return
            
        check_contains(self._text, g_datatypes, 'datatype not valid: %s' % self._text)
        return
    
    def get_name(self):
        return self._text

    def get_python_name(self):
        if self.is_string(): return 'str'
        if self.is_int(): return 'int'
        if self.is_double(): return 'float'
        if self.is_enum(): return 'str'
        if self._text == 'geo_pos_2d': return 'str'
        if self._text == 'geo_box_2d': return 'str'
        raise ValueError('Datatype not valid: %s' % datatype)

    def is_enum(self):
        return self._text.startswith('enum:')

    def is_ranged(self):
        return self._text.startswith('int:') or self._text.startswith('double:')
        
    def is_string(self):
        return self._text == 'string' or self._text.startswith('string:')
        
    def is_int(self):
        return self._text == 'int' or self._text.startswith('int:')

    def is_double(self):
        return self._text == 'double' or self._text.startswith('double:')

    def get_annotation(self):
        values = []
        if self.is_enum():
            values = self.enum_values
            text = ",".join([s for s in values])
            return 'enum_values: ' + text
        if self.is_ranged():
            text = str(self.min) + ',' + str(self.max)
            return 'range: ' + text

        return ''
    


class Param:
    name = None
    datatype = None
    description = None
    
    def __init__(self, name, datatype, description):
        check_is_instance(datatype, DataType, 'datatype type error: %s is not DataType' % name)
        self.name = name
        self.datatype = datatype
        self.description = description

    # In the JSON the switch name might be "output-file",
    # but that's not a legal Python name, so do a s/-/__/
    #
    # To simplify life, we do not support switch names
    # already containing "__". 
    def get_option_name(self):
        if '__' in self.name:
            raise ValueError("invalid option name: %s" % self.name)
        new_name = self.name.replace('-', '__')
        return new_name

    def get_full_description(self):
        s = self.description

        typ = self.datatype.get_name()
        if s != "": s += '\n'
        s += '# datatype: ' + typ
        
        anno = self.datatype.get_annotation()
        if anno != "": s += '\n' + '# ' + anno
        
        return s

            
class Parse:
    inputs = []
    outputs = []
    enums = {}
    ranges = {}
    name = ""
    description = {}

    def __init__(self, root):
        check_is_type(root, dict, 'Root is not a map')

        check_contains('name', root, 'root key "name" not found')
        check_is_string(root['name'], 'description is not a string')
        self.name = root['name']

        if 'description' in root:
            check_is_type(root['description'], dict, 'description is not a map')
            self.description = self.parse_description(root['description'])
        
        if 'enums' in root:
            self.enums = self.parse_enums(root['enums'])

        if 'ranges' in root:
            self.ranges = self.parse_ranges(root['ranges'])

        check_contains('inputs', root, 'root key "inputs" not found')
        self.inputs = self.parse_params(root['inputs'])

        check_contains('outputs', root, 'root key "outputs" not found')
        root['outputs']['_status'] = { 'datatype': 'int' }
        root['outputs']['_stdout'] = { 'datatype': 'string' }
        root['outputs']['_stderr'] = { 'datatype': 'string' }
        self.outputs = self.parse_params(root['outputs'])

    def parse_description(self, data):
        descr = { 'long_description': '', 'short': '', 'version': '' }
        if 'long_description' in data:
            check_is_string(data['long_description'], 'long_description is not a string')
            descr['long'] = data['long_description']
        if 'short_description' in data:
            check_is_string(data['short_description'], 'short_description is not a string')
            descr['short'] = data['short_description']
        if 'version' in data:
            check_is_string(data['version'], 'version is not a string')
            self.description['version'] = data['version']
        return descr
        
    def parse_enum_values(self, name, values):
        if len(values) == 0:
            raise ValueError('Enum values not specified')
        for value in values:
            check_is_string(value, 'enum value is not a string')
        return list(values)

    def parse_enums(self, enums):
        check_is_type(enums, dict, 'enums is not a map')
        es = {}
        for enum in enums:
            v = self.parse_enum_values(enum, enums[enum])
            es[enum] = v
        return es

    def parse_range_values(self, name, values):
        if len(values) != 2:
            raise ValueError('Incorrect number of range values')
        for value in values:
            if value == "+inf" or value == "-inf":
                pass
            elif type(value) is float:
                pass
            elif type(value) is int:
                pass
            else:
                raise TypeError('Unknown range value: %s' % value)
        return (values[0], values[1])
    
    def parse_ranges(self, ranges):
        check_is_type(ranges, dict, 'ranges is not a map')
        rs = {}
        for range in ranges:
            check_is_type(ranges[range], list, 'Range is not a list: %s' % range)
            r = self.parse_range_values(range, ranges[range])
            rs[range] = r 
        return rs
            
    def parse_param(self, name, data):
        check_is_type(data, dict, 'param is not a map')
        
        check_contains('datatype', data, 'param key "datatype" not found')
        datatype = DataType(data['datatype'], self.enums, self.ranges)
        
        description = ''
        if 'description' in data:
            description = data['description']
        
        return Param(name, datatype, description)
        
    def parse_params(self, data):
        check_is_type(data, dict, 'params is not a map')
        
        params = []
        for k in data:
            param = self.parse_param(k, data[k])
            params.append(param)
        return params

    def get_full_description(self):
        if len(self.description.values()) == 0: return ""
        return ", ".join(self.description.values())
        return s


class Script:
    _table = None
    _f = None
    
    def __init__(self, table):
        self._table = table
            
    def fprintf(self, str):
        self._f.write(str)

    def print_header_block(self):
        self.fprintf('#!/usr/bin/env python\n')
        self.fprintf('\n')
        self.fprintf('from geoserver.wps import process\n')
        self.fprintf('from com.vividsolutions.jts.geom import Geometry\n')
        self.fprintf('import re\n')
        self.fprintf('from subprocess import Popen, PIPE\n')
        self.fprintf('\n')
        self.fprintf('@process(\n')
        self.fprintf('    title = \'%s\',\n' % self._table.name)
        self.fprintf('    description = \'%s\',\n' % escapify(self._table.get_full_description()))
        
    def print_run_block(self):
        self.fprintf('def run(')
        if self._table.inputs:
            params = self._table.inputs
            params = sorted(params, key=lambda p: p.name)
            for param in params:
                self.fprintf(param.get_option_name())
                if param != params[-1]:
                    self.fprintf(', ')
        self.fprintf('):\n')

        self.fprintf('    cmd = "%s/%s.sh' % ("/tmp", self._table.name))
        if self._table.inputs:
            params = self._table.inputs
            params = sorted(params, key=lambda p: p.name)
            for param in params:
                self.fprintf(" --%s %s" % (param.get_option_name(), '%s'))
            self.fprintf("\" % (")
            params = self._table.inputs
            params = sorted(params, key=lambda p: p.name)
            for param in params:
                self.fprintf("%s" % (param.name))
                if param != params[-1]: self.fprintf(', ')
            self.fprintf(")")
        self.fprintf("\n")
        
        self.fprintf("""
    p = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
    out, err = p.communicate()
    print "Return code: ", p.returncode
    print out.rstrip(), err.rstrip()
    
    results = { "_stdout": out, "_stderr": err, "_status": p.returncode }
    p = re.compile('^\[(\w+)\]\s*(.*)')
    for line in out.split("\\n"):
        ms = p.findall(line)
        for (k,v) in ms:
            results[k] = v
    
    return results
""")

    def print_inputs_list(self):
        self.fprintf('    inputs = {\n')
        params = self._table.inputs
        params = sorted(params, key=lambda p: p.name)
        for param in params:
            param_name = param.get_option_name()
            datatype_name = param.datatype.get_python_name()
            description = escapify(param.get_full_description())
            self.fprintf('        \'%s\': (%s, \'\'\'%s\'\'\')' % (param_name, datatype_name, description))
            if param != params[-1]:
                self.fprintf(',')
            self.fprintf('\n')
        self.fprintf('    }')

    def print_outputs_list(self):
        self.fprintf("""    outputs = {
""")

        params = self._table.outputs
        params = sorted(params, key=lambda p: p.name)
        for param in params:

            param_name = param.get_option_name()
            datatype_name = param.datatype.get_python_name()
            description = escapify(param.get_full_description())
            self.fprintf('        \'%s\': (%s, \'\'\'%s\'\'\')' % (param_name, datatype_name, description))
            if param != params[-1]: self.fprintf(',\n')

        self.fprintf("""
    }
""")
        self.fprintf(')\n')

    def generate(self, filename):
        self._f = open(filename, "w")
        self.print_header_block()

        self.print_inputs_list()
        self.fprintf(',\n')

        self.print_outputs_list()
        self.fprintf('\n')

        self.print_run_block()
        self._f.close()



def main(jsonfile, pyfile):

    with io.open(jsonfile, 'r') as infile:
        print '============ %s ============' % os.path.basename(jsonfile)
        data = json.load(infile)
        table = Parse(data)
        
        #print '------------ json ------------'
        #print data

        print '------------ python ------------'
        script = Script(table)
        script.generate(pyfile)


if len(sys.argv) != 3:
    print 'Usage: $ %s file.json file.py' % os.path.basename(sys.argv[0])
    exit(1)

main(sys.argv[1], sys.argv[2])
