#include "tables.h"
#include <api/module.h>
#include <api/stdconfig.h>
#include <api/stddb.h>

mgr_db::JobCache *cache = nullptr;

mgr_db::JobCache *GetCache() {
	ASSERT(cache != nullptr, "Try to use db cache before it was initialized, add 'tables' to module dependencies");
	return cache;
}

User::User()
: Table("users", 63)
, Active(this, "active")
, Password(this, "passwd", 128)
, FullName(this, "fullname")
, IsAdmin(this, "is_admin") {
	Active.info().set_default("on");
	IsAdmin.info().set_default("off");
	Password.info().access_read = mgr_access::AccessNone;
}

Vote::Vote()
: Table("vote")
, Author(this, "users", mgr_db::rtCascade)
, Descr(this, "descr")
, RepDate(this, "repdate")
, Level(this, "level")
, Lector(this, "lector", "users", mgr_db::rtCascade) {
	Author.info().access_read = mgr_access::AccessNone;
	Author.info().access_write = mgr_access::AccessNone;
	Level.info().set_default(THEME_LEVEL_STANDARD);
	Lector.info().can_be_null = true;
	RepDate.info().can_be_null = true;
}

UserVote::UserVote()
: IdTable("uservote")
, User(this, "users", mgr_db::rtCascade)
, Vote(this, "vote", mgr_db::rtCascade)
, VoteFor(this, "vote_for") {
	AddIndex(mgr_db::itUnique, User, Vote);
}

Lector::Lector()
: IdTable("lector")
, User(this, "users", mgr_db::rtCascade)
, Vote(this, "vote", mgr_db::rtCascade)
, Ready(this, "is_ready") {
	AddIndex(mgr_db::itUnique, User, Vote);
}

namespace {
MODULE_INIT(tables, "") {
	mgr_cf::AddParam("VotePassword");
	mgr_db::ConnectionParams params;
	params.type = "mysql";
	params.host = "localhost";
	params.db = "vote";
	params.password = mgr_cf::GetParam("VotePassword");
	params.client = "vote";
	cache = new mgr_db::JobCache(params);
	cache->Register<User>();
	cache->Register<Vote>();
	cache->Register<UserVote>();
	cache->Register<Lector>();
	isp_api::RegisterComponent(cache);
	isp_api::SetDb(cache);
}
} // end of private namespace

