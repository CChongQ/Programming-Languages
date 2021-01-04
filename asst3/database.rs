/*
 * database.rs
 *
 * Implementation of EasyDB database internals
 *
 * University of Toronto
 * 2019
 */

use packet::{Command, Request, Response, Value};
use schema::Table;
use std::collections::HashMap;
 
/* OP codes for the query command */
pub const OP_AL: i32 = 1;
pub const OP_EQ: i32 = 2;
pub const OP_NE: i32 = 3;
pub const OP_LT: i32 = 4;
pub const OP_GT: i32 = 5;
pub const OP_LE: i32 = 6;
pub const OP_GE: i32 = 7;

/* You can implement your Database structure here
 * Q: How you will store your tables into the database? */
pub struct Database { 
    pub tables: HashMap<i32, DTable>,
}

pub struct DTable {
    pub data: Table,
    pub content: HashMap<i64, Row>,
    pub row_count: i64,
}

pub struct Row {
    pub version: i64,
    pub row_values: Vec<Value>
}

impl Database {
    pub fn new(table_schema: Vec<Table>) -> Database {

        let mut tables = HashMap::new();

        let mut i = 0;
        for each in table_schema {
            // println!("each = {:?} \n", each);
            
            // for eachT in &each.t_cols{
            //     println!("cols = {:?} \n", eachT.c_ref);
            // }

            tables.insert(
                (i+1) as i32,
                DTable {
                    data: each,
                    content: HashMap::new(),
                    row_count: 0,
                    
                }
            );
            i+=1;
        }

        Database {
            tables : tables,
        }
    }
}

/* Receive the request packet from client and send a response back */
pub fn handle_request(request: Request, db: & mut Database) 
    -> Response  
{           
    /* Handle a valid request */
    let result = match request.command {
        Command::Insert(values) => 
            handle_insert(db, request.table_id, values),
        Command::Update(id, version, values) => 
             handle_update(db, request.table_id, id, version, values),
        Command::Drop(id) => handle_drop(db, request.table_id, id),
        Command::Get(id) => handle_get(db, request.table_id, id),
        Command::Query(column_id, operator, value) => 
            handle_query(db, request.table_id, column_id, operator, value),
        /* should never get here */
        Command::Exit => Err(Response::UNIMPLEMENTED),
    };
    
    /* Send back a response */
    match result {
        Ok(response) => response,
        Err(code) => Response::Error(code),
    }
}


fn check_table(db: & Database, table_id: &i32) -> Result<i32, i32> {
    if !db.tables.contains_key(&table_id){ 
        return Err(Response::BAD_TABLE);
    }
    Ok(Response::OK)
}


fn check_row(db: & Database, table_id: &i32, values: & Vec<Value>)
    -> Result<i32, i32> {
    match check_table(&db, &table_id) {
        Ok(res) => res,
        Err(err) => return Err(err),
    };

    let temp = db.tables.get(&table_id).unwrap();
    let col_list = &temp.data.t_cols;

    if values.len() != col_list.len() {
         return Err(Response::BAD_ROW);
    }

    for (index, value) in values.iter().enumerate(){
        let col_info = &col_list[index];

        let value_type = match &value {
            Value::Null => Value::NULL,
            Value::Integer(_) => Value::INTEGER,
            Value::Float(_) => Value::FLOAT,
            Value::Text(_) => Value::STRING,
            Value::Foreign(v) => {
                if col_info.c_ref == 0 { 
                    println!("TEMP HERE");
                    return Err(Response::BAD_VALUE);
                }
                if !db.tables.contains_key(&col_info.c_ref) {
                    return Err(Response::BAD_FOREIGN);
                }

                let foreign_table = db.tables.get(&col_info.c_ref).unwrap();
                if !foreign_table.content.contains_key(&v) {
                    return Err(Response::BAD_FOREIGN);
                }
                Value::FOREIGN
            }
        };

        if value_type != Value::NULL {
            if value_type != col_info.c_type {
                return Err(Response::BAD_VALUE);
            }
        }
    }
    Ok(Response::OK)
}


/*
 * TODO: Implment these EasyDB functions
 */
 
fn handle_insert(db: & mut Database, table_id: i32, values: Vec<Value>) 
    -> Result<Response, i32> 
{
    // println!("============================\n");
    // println!("table_id = {}, values = {:?} \n",table_id, values);
    match check_row(&db, &table_id, &values) {
        Ok(res) => res,
        Err(err) => return Err(err),
    };

    // Process data
    let mut table = db.tables.get_mut(&table_id).unwrap(); //try to replace with mut...get(key).
    
    table.row_count += 1;

    let ver = 1;
    table.content.insert(
        table.row_count, 
        Row { version: ver, row_values: values }
    );

    Ok(Response::Insert(table.row_count, ver)) 
}

fn check_object(db: & Database, table_id: &i32, object_id: &i64, version: &i64) 
    -> Result<i32, i32> {
    let table = db.tables.get(&table_id).unwrap();
    if !table.content.contains_key(&object_id) {
        return Err(Response::NOT_FOUND);
    }

    let row = table.content.get(&object_id).unwrap();

    if *version == 0 || row.version == *version {
        return Ok(Response::OK);
    }
    Err(Response::TXN_ABORT)
}

fn handle_update(db: & mut Database, table_id: i32, object_id: i64, 
    version: i64, values: Vec<Value>) -> Result<Response, i32> 
{
    match check_row(&db, &table_id, &values) {
        Ok(res) => res,
        Err(err) => return Err(err),
    };

    match check_object(&db, &table_id, &object_id, &version) {
        Ok(res) => res,
        Err(err) => return Err(err),
    };

    let table = db.tables.get_mut(&table_id).unwrap();
    let mut row = table.content.get_mut(&object_id).unwrap();
    row.version += 1;
    row.row_values = values;
    Ok(Response::Update(row.version))
}

fn handle_drop(db: & mut Database, table_id: i32, object_id: i64) 
    -> Result<Response, i32>
{
    // println!("========================");
    // println!("tableID = {}, objectID = {}",table_id,object_id);
    match check_table(&db, &table_id) {
        Ok(res) => res,
        Err(err) => return Err(err),
    };
 
    let table = db.tables.get(&table_id).unwrap();
    if !table.content.contains_key(&object_id) {
        return Err(Response::NOT_FOUND);
    }

    let mut table_list: HashMap<i32,Vec<i64>> = HashMap::new(); 

    for (t_id, t_struct) in &db.tables {
        let cols = &t_struct.data.t_cols;
        for (o_id, o_struct) in &t_struct.content {
            for (index, o_value) in o_struct.row_values.iter().enumerate() {
                let temp = match o_value {
                    Value::Foreign(v) => v,
                    _=> &(-1 as i64),
                };
                if cols[index].c_ref == table_id {
                    if temp ==  &object_id {
                        match table_list.get_mut(t_id) {
                            Some(o) => o.push(*o_id),
                            None => {table_list.insert(*t_id,vec![*o_id]);}   
                        }
                    }
                }
            } 
        }
    }

    for (t_id, o_ids) in table_list {     
        for each_oid in o_ids {
            match handle_drop(db, t_id, each_oid) {
                Ok(res) => (),
                Err(err) => return Err(err),
            }
        }
    }
    
    let table_m = db.tables.get_mut(&table_id).unwrap();
    table_m.content.remove(&object_id);
    // Q: deal with row_count? 

    Ok(Response::Drop)
}

fn handle_get(db: & Database, table_id: i32, object_id: i64) 
    -> Result<Response, i32>
{
    match check_table(&db, &table_id) {
        Ok(res) => res,
        Err(err) => return Err(err),
    };

    let table_content = db.tables.get(&table_id).unwrap();
    if !table_content.content.contains_key(&object_id) {
        return Err(Response::NOT_FOUND);
    }

    let row = table_content.content.get(&object_id).unwrap();

    Ok(Response::Get(row.version,&row.row_values))

}

fn value_is_satisfied<T: Copy+PartialOrd+Eq>(operator: i32, i: T, j: T) 
    -> Result<bool, i32> {
    match operator {
        OP_EQ => Ok(i == j),
        OP_NE => Ok(i != j),
        OP_LT => Ok(i < j),
        OP_GT => Ok(i > j),
        OP_LE => Ok(i <= j),
        OP_GE => Ok(i >= j),
        _ => return Err(Response::BAD_QUERY),
    }
}

fn handle_query(db: & Database, table_id: i32, column_id: i32,
    operator: i32, other: Value) 
    -> Result<Response, i32>
{
    match check_table(&db, &table_id) {
        Ok(res) => res,
        Err(err) => return Err(err),
    };

    let table = db.tables.get(&table_id).unwrap();
    let col_list = &table.data.t_cols;
    if (col_list.len() as i32) < column_id {
        return Err(Response::BAD_QUERY);
    }

    let rows = &table.content;
    let mut result: Vec<i64> = Vec::new();

    match &other {
        Value::Foreign(_) => {
            if operator != OP_EQ && operator != OP_NE {
                return Err(Response::BAD_QUERY);
            }
        },
        _ => {},
    }

    if operator == OP_AL && column_id != 0 {
        return Err(Response::BAD_QUERY);
    }

    for (key, row) in rows.iter() {
        if column_id == 0 {
            match &other {
                Value::Integer(id) => {
                    match operator {
                        OP_EQ => {
                            if key == id { result.push(*key) }
                        },
                        OP_NE => {
                            if key != id { result.push(*key) }
                        },
                        _ => return Err(Response::BAD_QUERY),
                    }
                },
                Value::Null => result.push(*key),
                _ => return Err(Response::BAD_QUERY),
            }
        } else {
            let col_value = &row.row_values[(column_id-1) as usize];
            match (col_value, &other) {
                (Value::Integer(i), Value::Integer(j)) => {
                    match value_is_satisfied(operator, i, j) {
                        Ok(val) => {
                            if val {result.push(*key)}
                        },
                        Err(err) => return Err(Response::BAD_QUERY),
                    }
                },
                (Value::Float(i), Value::Float(j)) => {
                    match operator {
                        // float does not support equal operator
                        OP_NE => {
                            if i != j { result.push(*key) }
                        },
                        OP_LT => {
                            if i < j { result.push(*key) }
                        },
                        OP_GT => {
                            if i > j { result.push(*key) }
                        },
                        OP_LE => {
                            if i <= j { result.push(*key) }
                        },
                        OP_GE => {
                            if i >= j { result.push(*key) }
                        },
                        _ => return Err(Response::BAD_QUERY),
                    }
                },
                (Value::Text(i), Value::Text(j)) => {
                    match value_is_satisfied(operator, i, j) {
                        Ok(val) => {
                            if val {result.push(*key)}
                        },
                        Err(err) => return Err(Response::BAD_QUERY),
                    }
                },
                (Value::Foreign(i), Value::Foreign(j)) => {
                    match operator {
                        OP_EQ => {
                            if i == j { result.push(*key) }
                        },
                        OP_NE => {
                            if i != j { result.push(*key) }
                        },
                        _ => return Err(Response::BAD_QUERY),
                    }
                },
                _ => return Err(Response::BAD_QUERY),
            }
        }
    }
    Ok(Response::Query(result))
}

