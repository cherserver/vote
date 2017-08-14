#include <api/dbaction.h>
#include <mgr/mgrstr.h>
#include "tables.h"

namespace {
using namespace isp_api;
class ActionUser : public ExtTableNameListAction<User> {
public:
	ActionUser() : ExtTableNameListAction("user", AccessAdmin, *GetCache()) {}

	virtual void TableSet(Session &ses, Cursor &cur) const {
		if (cur->Name != cur->Name.OldValue() && isp_api::UserExists(cur->Name))
			throw mgr_err::Exists("user", cur->Name);

		if (!cur->Password.empty())
			cur->Password = str::Crypt(cur->Password);
		else if (!cur->IsNew())
			cur->Password = cur->Password.OldValue();
	}
};

MODULE_INIT(user, "tables") {
	new ActionUser;
}

} // end of private namespace

