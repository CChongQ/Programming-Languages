#!/usr/bin/python3
#
# fields.py
#
# Definitions for all the fields in ORM layer
#

from datetime import datetime

class Field:
    def __init__(self, blank=False, default=None, choices=None):
       
        if choices is not None:
            for choice in choices:
                self.check_type(choice)
            
        if default is not None: 
            if choices is not None and default not in choices:
                raise TypeError
            self.check_type(default)
            
        self.blank = blank
        self.default = default
        self.choices = choices
        self._values = {}
    
    def __get__(self, inst, owner):
        return self._values[inst]

    def __set__(self, inst, value):
        if inst in self._values and value is None:
            raise AttributeError
        elif value is None:
            if not self.blank and self.default is None:
                raise AttributeError
            value = self.default   

        if value is not None:
            self.check_type(value)
            value = self.parser(value)
        if self.choices is not None:
            if self.choices and value not in self.choices:
                raise ValueError
        self._values[inst] = value
    
    @classmethod
    def check_type(cls, value):
        pass

    def parser(self, value):
        return value

class Integer(Field):   
    def __init__(self, blank=False, default=0, choices=None):
        super().__init__(blank, default, choices)
    
    @classmethod
    def check_type(cls, value):
        if type(value) is not int:
            raise TypeError

class Float(Field): 
    def __init__(self, blank=False, default=0., choices=None):
        super().__init__(blank, default, choices)
    
    @classmethod
    def check_type(cls, value):
        if type(value) not in (int, float):
            raise TypeError
    
    def parser(cls, value):
        return float(value)

class String(Field):
    def __init__(self, blank=False, default=None, choices=None):
        super().__init__(blank, default, choices)
    
    @classmethod
    def check_type(cls, value):
        if type(value) is not str:
            raise TypeError

class Foreign(Field):
    def __init__(self, table, blank=False):
        super().__init__(blank)
        self.table = table

    def check_type(self, value):
        if type(value) is not self.table:
            if value is None and self.blank:
                pass
            else:
                raise TypeError

class DateTime(Field):
    implemented = True
    
    def __init__(self, blank=False, default=None, choices=None):
        if default is not None:
            if default==0:
                 default = datetime.fromtimestamp(0)
            else:
                default = default()
        else:
            default = datetime.fromtimestamp(0)
        
        super().__init__(blank,default,choices)
    
    @classmethod
    def check_type(cls, value):
        if type(value) is not datetime:
            raise TypeError

class Coordinate(Field):
    implemented = True

    def __init__(self, blank=False, default=None, choices=None):
        super().__init__(blank,default,choices)
    
    @classmethod
    def check_type(cls, value):
        if value is not None:
            type_allow = [int, float]
            if len(value) != 2 or type(value) is not tuple:
                raise TypeError
            if type(value[0]) not in type_allow or type(value[1]) not in type_allow:
                raise ValueError
            if (value[0] < -90 or value[0]>90) or (value[1]<-180 or value[1]>180):
                raise ValueError
