#include "node_db.hxx"

NodeDB &NodeDB::GetInstance() {
	static NodeDB *db = new NodeDB;
	return *db;
}

NodeDB::NodeDB() {
}
NodeDB::~NodeDB() {
}