#ifndef __TABLES_H__
#define	__TABLES_H__
#include <mgr/mgrdb_struct.h>

#define THEME_LEVEL_NOVICE "novice"
#define THEME_LEVEL_STANDARD "standard"
#define THEME_LEVEL_PRO "pro"

mgr_db::JobCache *GetCache();

class User : public mgr_db::Table {
public:
	mgr_db::BoolField Active;
	mgr_db::StringField Password;
	mgr_db::StringField FullName;
	mgr_db::BoolField IsAdmin;
	User();
};

class Vote : public mgr_db::Table {
public:
	Vote();
	mgr_db::ReferenceField Author;
	mgr_db::TextField Descr;
	mgr_db::DateTimeField RepDate;
	mgr_db::StringField Level;
	mgr_db::ReferenceField Lector;
};

class UserVote : public mgr_db::IdTable {
public:
	UserVote();
	mgr_db::ReferenceField User;
	mgr_db::ReferenceField Vote;
	mgr_db::BoolField VoteFor;
};

class Lector : public mgr_db::IdTable {
public:
	Lector();
	mgr_db::ReferenceField User;
	mgr_db::ReferenceField Vote;
	mgr_db::BoolField Ready;
};

#endif

