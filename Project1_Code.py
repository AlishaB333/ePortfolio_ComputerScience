from pymongo import MongoClient 
from bson.objectid import ObjectId


class AnimalShelter: 
    """ CRUD operations for Animal collection in MongoDB """ 

    def __init__(self): 
        # Initializing the MongoClient. This helps to access the MongoDB 
        # databases and collections. This is hard-wired to use the aac 
        # database, the animals collection, and the aac user. 
        # 
        # You must edit the password below for your environment. 
        # 
        # Connection Variables 
        # 
        USER = 'aacuser' 
        PASS = 'SNHU1293' 
        HOST = 'localhost' 
        PORT = 27017 
        DB = 'aac' 
        COL = 'animals' 
        # 
        # Initialize Connection 
        # 
        self.client = MongoClient('mongodb://%s:%s@%s:%d' % (USER,PASS,HOST,PORT)) 
        self.database = self.client['%s' % (DB)] 
        self.collection = self.database['%s' % (COL)] 
            
    #Method that inserts a document into database
    def create(self, data):
        
        try:
            result = self.collection.insert_one(data)
            return True if result.inserted_id is not None else False
        except Exception as e:
            print("Error while inserting document:", e)
            return False

    #Method that queries for documents from database
    def read(self, query):
        
        try:
            cursor = self.collection.find(query, {'_id':False})
            return list(cursor)
        except pymongo.errors.PyMongoError as e:
            print("Error while reading documents:", e)
            return []
        
    #Method that updates a dataset
    def update(self, query, new_data):
        try:
            result = self.collection.update_many(query, new_data)
            if result.modified_count > 0:
                return result.modified_count
            else:
                return print("No Modified records")
        except pymongo.errors.PyMongoError as e:
            print("No query provided for update:", e)
            return []
    
    #Method that deletes a dataset
    def delete(self, data):
        try:
            result = self.collection.delete_one(data)
            if result.deleted_count > 0:
                return result.deleted_count
            else:
                return []
        except pymongo.errors.PyMongoError as e:
            print("No data provided for delete:", e)
            return []