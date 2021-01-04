#!/usr/bin/python3
#
# easydb.py
#
# Definition for the Database class in EasyDB client
#
import struct
import socket
from .packet import *
from .exception import *


STRING = str
COLUMN_TYPE = [str,float,int]

# column types
NULL = 0
INTEGER = 1
FLOAT = 2
STRING = 3
FOREIGN = 4

PACK_FORMAT = {'int': '>q','float': '>d',}
COL_TYPE_CODE = {'int':INTEGER, 'str':STRING, 'float':FLOAT, 'foreign': FOREIGN}
operator_dict = list(vars(operator).values())[1:8] 

s = socket.socket() 

class Database:
 

    def __repr__(self):
        return "<EasyDB Database object>"

            
    def checkNameFormat(self,name):
        #illegal name format
        if name[0].isalpha()==False or name=='id':
            return False  
    
    def __init__(self, tables):

        self.tablesInfo = {} # a dict for schema 
        self.tableIndexDict = {}
        
        try: 
            iter(tables)
        except:
            raise TypeError

        for index, eachTb in enumerate(tables):

            tableName = eachTb[0]
            tableColumns = eachTb[1]
           
            #Check Table Name
            if type(tableName) is not str:
                #illegal name type
                raise TypeError
            if tableName in self.tablesInfo.keys():
                # duplicate table name
                raise ValueError
            if tableName[0].isalpha() == False:
                raise ValueError
                
            
            #Check Column Name
            colNames = set() #a set of column name for current table
            colInfo = []
            for eachCol in tableColumns:
                if type(eachCol[0]) is not str:
                    #illegal column name
                    raise TypeError
                elif self.checkNameFormat(eachCol[0]) ==False:
                    raise ValueError
                elif (eachCol[1] not in COLUMN_TYPE):
                    if type(eachCol[1]) is str:
                        #foreign key 
                        if  eachCol[1] not in self.tablesInfo.keys() or eachCol[1] == tableName:
                            raise IntegrityError
                    else:
                        #illegal column type
                        raise ValueError
                elif eachCol[0] in colNames:
                    #duplicate column names
                    raise ValueError
                colNames.add(eachCol[0])
                colInfo.append(eachCol)
            
            self.tablesInfo[tableName]= colInfo
            self.tableIndexDict[tableName] = index

        pass
    

    def connect(self, host, port):
        try:
            s.connect((host,int(port)))
            con_response = s.recv(4096)
            con_red_unpack = struct.unpack_from('>i',con_response)
            recv_code = con_red_unpack[0]
            if  recv_code == SERVER_BUSY:
                self.close()
                return False
            return True 
        except Exception as e:
            self.close()
            return False


    def close(self):
        pack_exit = struct.pack('>ii',EXIT,1)
        s.send(pack_exit)  
        pass
    
    def check_row(self,table_name,values):
        #value type = column type
        for i,eachCol in enumerate(values):
            if (type(eachCol) is not self.tablesInfo[table_name][i][1]) and (self.tablesInfo[table_name][i][1] not in self.tablesInfo.keys()):       
                raise PacketError 
        return

    def requestStruct(self, command,tableID):
        return struct.pack('>ii',command,tableID)
    
    def valueStruct(self,table_name,eachV,i,special=False):
        if special == True:
            #for SCAN function
            colType = NULL
            bufSize = 0
        elif i==-1:
            i+=1
            colType = table_name
            bufSize = 8
        else:
            colType = self.tablesInfo[table_name][i][1]
            bufSize = -1
            #size of the buf: 
            if type(eachV) is int  or type(eachV) is float:
                bufSize = 8
            elif type(eachV) is str:
                if len(eachV)%4 == 0:
                    bufSize = len(eachV) 
                else:
                    bufSize = (len(eachV)//4 +1)*4     
            else:
                bufSize = 0

        value_pack = 0
        col_type = 0
        
        if colType is str: 
            value_pack_en = eachV.encode('ascii')
            value_pack = struct.pack('%ds'%bufSize,value_pack_en)
            col_type = COL_TYPE_CODE[colType.__name__]
        elif colType in self.tablesInfo.keys():
            #foreign key
            if type(eachV) is not int:
                raise PacketError
            value_pack = struct.pack('>q',eachV)
            col_type = COL_TYPE_CODE["foreign"]
        elif colType is NULL:
            value_pack = None
            col_type = NULL
        else:
            temp_f = PACK_FORMAT[colType.__name__]
            value_pack = struct.pack(temp_f,eachV) 
            col_type = COL_TYPE_CODE[colType.__name__]

        inner_pack = struct.pack('>ii',col_type,bufSize)
        
        return inner_pack,value_pack

    def rowStruct(self, table_name,values):
       
        numEle = len(values)
        pack_count = struct.pack('>i',numEle)

        value_total=[None]*len(values)
        for i,eachV in enumerate(values):         
            inner_pack,value_pack = self.valueStruct(table_name,eachV,i)   
            value_total[i] = b''.join([inner_pack,value_pack])     
        temp = b''.join(value_total)

        rowResult = b''.join([pack_count,temp])
        return rowResult
            
    def check_input(self,table_name,values):
        #check: 1. table name does not exist 2 . len(value) > column number 
        if table_name not in self.tablesInfo.keys():
            raise PacketError
        elif len(values) != len(self.tablesInfo[table_name]):
            raise PacketError   


    def insert(self, table_name, values): 
        


        self.check_input(table_name, values)                   
        self.check_row(table_name,values)

        tableID = self.tableIndexDict[table_name] +1  #id of the first table in the database is 1, rather than 0
        pack_command = self.requestStruct(INSERT,tableID)

        #pack row
        pack_row= self.rowStruct(table_name,values)
        pack_com_row = b''.join([pack_command,pack_row])

        s.send(pack_com_row)

        recv_response = s.recv(4096)
        if len(recv_response) == 4:
            error_code, = struct.unpack('>i',recv_response)
            self.errorCheck(error_code)
        else: 
            resp_unpack = struct.unpack_from('>iqq',recv_response,0)
            recv_code = resp_unpack[0]
            if recv_code == OK:
                pk = resp_unpack[1]
                version = resp_unpack[2]
                return pk,version
            else:
                self.errorCheck(recv_code)
                return  

    def update(self, table_name, pk, values, version=0):
        
        if version == None:
            version = 0
       
        if type(pk) is not int or type(version) is not int:
            raise PacketError

        self.check_input(table_name,values)
        
        tableID = self.tableIndexDict[table_name] + 1 
        pack_command = self.requestStruct(UPDATE,tableID)
      
        #pack key
        pack_key = struct.pack('>qq',pk,version)
       
        #pack row
        self.check_row(table_name,values)    
        pack_row= self.rowStruct(table_name,values)
        pack_com_key_row = b''.join([pack_command,pack_key,pack_row])
       
        s.send(pack_com_key_row)

        recv_response = s.recv(4096)
        if len(recv_response) == 4:
            error_code, = struct.unpack('>i',recv_response)
            self.errorCheck(error_code)
        else:
            resp_unpack = struct.unpack_from('>iq',recv_response,0)
            recv_code = resp_unpack[0]
            if recv_code == OK:
                version = resp_unpack[1]
                return version
            else:
                #foreign key not found
                self.errorCheck(recv_code)

        pass

    def drop(self, table_name, pk):
     
        if type(pk) is not int :
            raise PacketError
        if table_name not in self.tablesInfo.keys():
            raise PacketError
        
        tableID = self.tableIndexDict[table_name] + 1 
        pack_command = self.requestStruct(DROP,tableID)

        pack_id = struct.pack('>q',pk)
        pack_com_id = b''.join([pack_command,pack_id])
        
        s.send(pack_com_id)

        recv_response = s.recv(4096)
        if len(recv_response) == 4:
            error_code, = struct.unpack('>i',recv_response)
            self.errorCheck(error_code)

        pass
        
    def get(self, table_name, pk):
       
        if type(pk) is not int :
            raise PacketError
        if table_name not in self.tablesInfo.keys():
            raise PacketError

        tableID = self.tableIndexDict[table_name] + 1 
        pack_command = self.requestStruct(GET,tableID)

        pack_id = struct.pack('>q',pk)

        pack_com_pk = b''.join([pack_command,pack_id])

        s.send(pack_com_pk)

        recv_response = s.recv(4096)
        if len(recv_response) == 4:
            error_code, = struct.unpack('>i',recv_response)
            self.errorCheck(error_code)
        else:
            #decode values
            info_list = struct.unpack_from('>iqi', recv_response, 0)
            version = info_list[1]
            cut_1 = struct.calcsize('>i')*2 + struct.calcsize('>q') #iqi
            remain = recv_response[cut_1:]
            count = info_list[2]
            result_list=[]

            while count >0:
                cut_2 = struct.calcsize('>i')*2 
                c_type,c_size,= struct.unpack_from('>ii', remain[:cut_2], 0)
                fmt = '>ii'+'%ds'%c_size
                temp = remain[:cut_2+c_size]
                c_type, c_size,value = struct.unpack_from(fmt, temp, 0)
               
                if c_type == STRING:
                    value = value.rstrip(b'\x00')
                    dec_val = value.decode('ascii')
                elif c_type == FLOAT:
                    dec_val, = struct.unpack_from('>d',value)
                elif c_type == INTEGER or c_type == FOREIGN:
                    dec_val, = struct.unpack_from('>q',value)
                else:
                    print('Shold not be here')
                
                result_list.append(dec_val)
                remain = remain[cut_2+c_size:]
                count-=1

            value = result_list

        return value,version

        pass

    def scan(self, table_name, op, column_name=None, value=None):

        try:
            temp = self.tablesInfo[table_name]
        except Exception as e:
            raise PacketError 

        if op not in operator_dict:
            raise PacketError
        
       
        columnID = -2
        specialCase = False
        col_plus = False
        if column_name == 'id':
            if op != operator.EQ and op != operator.NE :
                raise PacketError

            columnID = -1 
        elif (op == operator.AL):
            columnID = -1
            specialCase = True
        else:
            if column_name == None:
                raise PacketError
            else:
                for index,eachCol in enumerate(self.tablesInfo[table_name]):
                    if eachCol[0] == column_name:
                        if eachCol[1] in self.tablesInfo.keys() and (op != operator.EQ and op != operator.NE):
                            raise PacketError
                        else:
                            columnID = index
                            col_plus = True
                        break
        
        if columnID == -2:
            #column not found
            raise PacketError 
        
        if columnID != -1:
            colType = self.tablesInfo[table_name][columnID][1]
            if colType in COLUMN_TYPE and type(value) is not colType:
                raise PacketError

        tableID = self.tableIndexDict[table_name] + 1 
        pack_command = self.requestStruct(SCAN,tableID)

        # if col_plus==False :
        #     pack_op = struct.pack('>ii',columnID,op)
        # else:
        pack_op = struct.pack('>ii',columnID+1,op)

       
        val_inner,value_temp= self.valueStruct(table_name,value,columnID,special = specialCase) 
        if value_temp != None:
            pack_value= b''.join([val_inner,value_temp])
        else:   
            pack_value = val_inner

        pack_com_op = b''.join([pack_command,pack_op,pack_value])

        s.send(pack_com_op)

        recv_response = s.recv(4096)
        result = []
        if len(recv_response) == 4:
            error_code, = struct.unpack('>i',recv_response)
            self.errorCheck(error_code)
        else:
            if len(recv_response) == 8:
                #no id found
                result = []
            else:
                resp_unpack = struct.unpack_from('>iiq',recv_response,0)
                recv_code = resp_unpack[0]
                count = resp_unpack[1]
                if recv_code == OK:
                    fmt = '>ii'+'q' * count
                    resp_unpack_true = struct.unpack_from(fmt,recv_response,0)
                    result = list(resp_unpack_true[2:])                 
                else:
                    self.errorCheck(recv_code)

        return result
        pass
    
    
    def errorCheck(self,error_code):
        if error_code == BAD_FOREIGN:
            raise InvalidReference
        if error_code == TXN_ABORT:
            raise TransactionAbort
        if error_code == NOT_FOUND:
            raise ObjectDoesNotExist
        if error_code == BAD_REQUEST:
            raise IndexError
        if error_code == BAD_QUERY:
            raise InvalidReference
   

                        
