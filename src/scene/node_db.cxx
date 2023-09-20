#include "node_db.hxx"

NodeDB &NodeDB::get() {
	static NodeDB *db = new NodeDB;
	return *db;
}

NodeDB::NodeDB() {
}
NodeDB::~NodeDB() {
}