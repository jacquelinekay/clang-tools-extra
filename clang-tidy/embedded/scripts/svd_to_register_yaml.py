from svd_parser import format_utils
from svd_parser import parser_utils

import copy
from sys import argv
import yaml

class HexInt(int): pass
def representer(dumper, data):
    return yaml.ScalarNode('tag:yaml.org,2002:int', hex(data))


if __name__ == '__main__':
    if len(argv) < 3:
        print('Usage: svd_to_register_yaml.py <input_file> <output_file>')
        exit(1)

    input_filename, output_filename = argv[1], argv[2]

    register_map = []

    device_properties = parser_utils.parse_properties_from_xml(input_filename)

    for peripheral in device_properties.find_all('peripheral'):
        for register in format_utils.get_registers(peripheral):
            register_map.append({})
            cur_reg = register_map[-1]
            cur_reg['address'] = format_utils.sanitize_int(str(register.addressOffset.string), 0) + format_utils.sanitize_int(str(peripheral.baseAddress.string), 0)
            cur_reg['name'] = format_utils.format_register_name(peripheral, register).encode('utf8')
            cur_reg['fields'] = []
            register_fields = register.find_all('field')
            if len(register_fields) == 0:
                register_fields.append(format_utils.expand_register_as_field(register, device_properties))
            for field in register_fields:
                register_map[-1]['fields'].append({})
                cur_field = cur_reg['fields'][-1]
                cur_field['name'] = format_utils.format_field_name(field).encode('utf8')
                cur_field['mask'] = format_utils.setBitsFromRange(format_utils.msb(field), format_utils.lsb(field))
                if format_utils.use_enumerated_values(field):
                    cur_field['values'] = []
                    for v in field.find_all('enumeratedValue'):
                        cur_field['values'].append({})
                        cur_value = cur_field['values'][-1]
                        # TODO Make sure this offset is correct
                        cur_value['value'] = int(format_utils.format_enum_value(v), 0)
                        cur_value['name'] = format_utils.format_enum_value_name(v).encode('utf8')

    with open(output_filename, 'w+') as f:
        yaml.add_representer(int, representer)
        output = yaml.dump(register_map, f, explicit_start=True, canonical=False, default_flow_style=False, Dumper=yaml.Dumper, encoding='utf8')

