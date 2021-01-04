#!/usr/bin/python3
#
# table.py
#
# Definition for an ORM database table and its metaclass
#

# metaclass of table
# Implement me or change me. (e.g. use class decorator instead)

from collections import OrderedDict
from datetime import datetime
from .easydb import operator
from .field import Field, Integer, Float, String, Foreign, DateTime, Coordinate

OP_DICT = {"ne": operator.NE, "gt": operator.GT, "lt": operator.LT, 'eq': operator.EQ,'al':operator.AL}

class MetaTable(type):

    table_dict = [] #To have a ordered table dict s
   
    def __prepare__(mcls, bases):
        return OrderedDict()

    def __init__(cls, name, bases, attrs):
        pass
     
    def __new__(mcs, cls_name, bases, kwargs):
        
        if any( cls_name== i.__name__ for i in mcs.table_dict): #duplicate 
            raise AttributeError

        cls = super().__new__(mcs, cls_name, bases, kwargs)

        if cls_name != "Table":
            temp_f = []
            temp_name = []
            
            for name_att, type_att in kwargs.items():
                if isinstance(type_att, Field):
                    special_w = ['pk','version','save','delete']
                    has_underscore = name_att.__contains__("_")
                    if name_att in special_w or name_att in temp_name or has_underscore:
                        raise AttributeError
                    
                    temp_f.append((name_att, type_att))
                    temp_name.append(name_att)
            
            cls._fields = temp_f
            mcs.table_dict.append(cls)

        return cls
    

    # Returns an existing object from the table, if it exists.
    #   db: database object, the database to get the object from
    #   pk: int, primary key (ID)
    def get(cls, db, pk):

        values, version = db.get(cls.__name__,pk)
        kwargs = {}  
        index = 0
        for f_name, obj in cls._fields:
            if isinstance(obj,Foreign):
                object=[]
                for eachC in MetaTable.table_dict:
                    if eachC.__name__ == obj.table.__name__:
                        object = eachC
                f_obj= object.get(db,values[index])
                kwargs[f_name] = f_obj
                index+=1
            elif type(obj) is Coordinate:
                kwargs[f_name] = (values[index], values[index+1]) #2-tuple -> 2 values
                index+=2
            elif type(obj) is DateTime:
                kwargs[f_name] = datetime.fromtimestamp(values[index]) #float -> datetime
                index +=1
            else:
                kwargs[f_name] = values[index]
                index+=1

        result = cls(db, **kwargs)
        result.pk = pk
        result.version = version
        
        return result

    # Returns a list of objects that matches the query. If no argument is given,
    # returns all objects in the table.
    # db: database object, the database to get the object from
    # kwarg: the query argument for comparing
    def filter(cls, db, **kwarg):

        result = []

        if len(kwarg) ==0:
            #Case: empty input
            pks = db.scan(cls.__name__, OP_DICT['al'])
            result = [cls.get(db,eachPk) for eachPk in pks] 
        else:
            for c_op, value in kwarg.items():
                if '__' in c_op:
                    columnname,op = c_op.split('__')
                    if op not in OP_DICT.keys() or not(columnname == 'id' or any(columnname == f_name for f_name,obj in cls._fields)):
                        raise AttributeError             
                else:
                    #Case: eq and foreign 
                    columnname = c_op
                    op = 'eq'
                    if isinstance(value,Table):
                        value = value.pk                 
                
                if type(value) is tuple: #Coordinate
                    pk1 = db.scan(cls.__name__, OP_DICT[op],columnname+'_lat',value[0])
                    pk2 = db.scan(cls.__name__, OP_DICT[op],columnname+'_long',value[1])
                    pks = sorted(set(pk1).intersection(set(pk2)))
                else:
                    if isinstance(value,datetime):
                        value = value.timestamp()  #datetime -> float       
                    pks = db.scan(cls.__name__, OP_DICT[op],columnname,value)
               
                result = [cls.get(db,eachPk) for eachPk in pks] 

        return result

    # Returns the number of matches given the query. If no argument is given, 
    # return the number of rows in the table.
    # db: database object, the database to get the object from
    # kwarg: the query argument for comparing
    def count(cls, db, **kwarg):
     
        obj_list = cls.filter(db, **kwarg)
        result = len(obj_list)
       
        return result

# table class
# Implement me.
class Table(object, metaclass=MetaTable):

    def __init__(self, db, **kwargs):
        self.pk = None      # id
        self.version = None # version
        self._db = db
        self._table_name = self.__class__.__name__

        fields = self.__class__._fields
        field_names = []
        for name, obj in fields:
            should_be_specified = not obj.blank and obj.default is None

            if name in kwargs:
                setattr(self, name, kwargs[name])
            else:
                setattr(self, name, None)
                if should_be_specified:
                    raise AttributeError
            field_names.append(name)
 
        self._field_names = field_names

    def _get_field_values(self, format=False):
        values = []
        for name in self._field_names:
            value = getattr(self, name)
            
            if format:
                if isinstance(value, Table):
                    values.append(value.pk)
                else:
                    if type(value) is tuple: #coordinate 
                        values.append(value[0])
                        values.append(value[1])
                    else:
                        if type(value) is datetime:
                            value = value.timestamp()
                        values.append(value)
            else:
                values.append(value)

        return values

    # Save the row by calling insert or update commands.
    # atomic: bool, True for atomic update or False for non-atomic update
    def save(self, atomic=True):
        values = self._get_field_values()

        for value in values:
            if isinstance(value, Table): # check foreign key
                if value.pk is None: # save the referenced obj first
                    value.save(atomic)

        formatted_field_values = self._get_field_values(format=True)
        if self.pk is None:
            self.pk, self.version = self._db.insert(self._table_name, formatted_field_values)
            
        else:
            args = [self._table_name, self.pk, formatted_field_values]
            if atomic:
                args.append(self.version)
            self.version = self._db.update(*args)

    # Delete the row from the database.
    def delete(self):
        self._db.drop(self._table_name, self.pk)
        self.pk = None
        self.version = None

