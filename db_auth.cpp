#include <api/module.h>
#include <api/auth_method.h>
#include <mgr/mgrstr.h>
#include "tables.h"

namespace {
using namespace isp_api;

class SimpleAuth : public isp_api::AuthMethod {
public:
	SimpleAuth() : isp_api::AuthMethod("simple") {}

	void Fillup(mgr_xml::Xml &xml, std::shared_ptr<User> &user) const {
		auto node = xml.GetRoot().AppendChild("ok");
		node.SetProp("level", user->IsAdmin ? str::Str(lvAdmin) : str::Str(lvUser));
		node.SetProp("name", user->Name);
		node.SetProp("method", this->name());
		node.AppendChild("ext", user->Id).SetProp("name", "uid");

	}
	virtual void AuthenByName(mgr_xml::Xml &res, const string &name) const {
		auto cur = GetCache()->Get<User>();
		if (cur->FindByName(name) && cur->Active)
			Fillup(res, cur);
	}
	virtual void AuthenByPassword(mgr_xml::Xml &res, const string &name, const string &pass) const {
		auto cur = GetCache()->Get<User>();
		if (cur->FindByName(name) && cur->Active && str::Crypt(pass, cur->Password) == cur->Password)
			Fillup(res, cur);
	}
	virtual bool Find(const string &name) const {
		auto cur = GetCache()->Get<User>();
		return cur->FindByName(name);
	}
	virtual void SetPassword(const string &user, const string &pass) const {
		auto cur = GetCache()->Get<User>();
		cur->AssertByName(user);
		cur->Password = str::Crypt(pass);
		cur->Post();
	}
};

MODULE_INIT(auth, "tables") {
	new SimpleAuth;
}
} // end of private namespace

