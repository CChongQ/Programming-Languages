#!/usr/bin/python3
#
# orm.py
#
# Definition for setup and export function
#

from .easydb import Database

from .field import Field, Integer, Float, String, Foreign, DateTime, Coordinate

import importlib, inspect

Type_dict = {String: str,Float: float, Integer: int}
Type_dict_string = {str: 'string', float: 'float', int:'integer'}


# Return a database object that is initialized, but not yet connected.
#   database_name: str, database name
#   module: module, the module that contains the schema
def setup(database_name, module):
    # Check if the database name is "easydb".
    if database_name != "easydb":
        raise NotImplementedError("Support for %s has not implemented"%(
            str(database_name)))

    schema = createSchema(module)
    return Database(schema)

# Return a string which can be read by the underlying database to create the 
# corresponding database tables.
#   database_name: str, database name
#   module: module, the module that contains the schema
def export(database_name, module):

    # Check if the database name is "easydb".
    if database_name != "easydb":
        raise NotImplementedError("Support for %s has not implemented"%(
            str(database_name)))

    # IMPLEMENT ME
    schema_tuple = createSchema(module)
    result_string = ''

    # for eachTable in schema_tuple:
    for t_Name, t_attr in schema_tuple:
        a_string = ''
        for eachA in  t_attr:
            a_type = ''
            if type(eachA[1]) is str: 
                a_type  = eachA[1]
            else:
                a_type = Type_dict_string[eachA[1]]
            temp = eachA[0] + ' : ' + a_type + ' ; '
            a_string+=temp
        result_string+= t_Name + ' { ' + a_string + '} '

    return result_string

def createSchema(module):
    result = []
    
    #Get all classes
    table_ordered = []

    for i,j in inspect.getmembers(module,inspect.isclass): #Note:the result of getmembers are not ordered
        if type(j) is not type:
            table_ordered = j.table_dict
    
    for eachCls in table_ordered:
        attr = []
        for f_name, f_type in eachCls._fields:

            if type(f_type) is Coordinate:
                t_type = float 
                attr.append((f_name+"_lat", t_type))
                attr.append((f_name+"_long", t_type))
            elif type(f_type) is DateTime:
                t_type = float 
                attr.append((f_name, t_type))
            elif type(f_type) is Foreign:
                t_type = f_type.table.__name__ 
                attr.append((f_name, t_type))
            else:
                t_type = Type_dict[type(f_type)]
                attr.append((f_name, t_type))

        result.append((eachCls.__name__,attr))

    return result

