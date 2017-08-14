#include <api/dbaction.h>
#include <api/module.h>
#include "tables.h"

namespace priv_vote {
using namespace isp_api;

AccessMask DefaultRights() {
	return EqLevel(lvUser) + EqLevel(lvAdmin);
}

string GetAdmins() {
	string admins;
	ForEachQuery(GetCache(),
		"SELECT fullname"
		" FROM users"
		" WHERE is_admin='on'"
		" ORDER BY fullname"
		, q) {
		admins = str::Append(admins, q->AsString(0), ", ");
	}

	return admins;
}

class VoteAction : public TableIdListAction<Vote> {
public:
	VoteAction()
	: TableIdListAction("vote", DefaultRights(), *GetCache())
	, m_up(this, "up", true)
	, m_down(this, "down", false)
	, m_be_lector(this)
	, m_not_be_lector(this)
	, m_set_date(this)
	{}
private:
	void List(Session &ses) const {
		auto user_cur = GetCache()->Get<User>(ses.auth.ext("uid"));
		ForEachQuery(GetCache(),
				"SELECT v.id AS id"                     //0
				", v.name AS name"                      //1
				", v.descr AS descr"                    //2
				", u.fullname AS author"                //3
				", uvm.vote_for AS myvote"              //4
				", COUNT(uvp.id) AS pro"                //5
				", COUNT(uvc.id) AS con"                //6
				", COUNT(uvp.id)-COUNT(uvc.id) AS mark" //7
				", COUNT(l.id) AS lectors"              //8
				", ml.id AS melector"                   //9
				", u.id AS author_id"                   //10
				", v.level AS level"                    //11
				", lu.fullname AS lector"               //12
				", v.repdate AS repdate"                //13
				" FROM vote v"
				" INNER JOIN users u ON u.id=v.users"
				" LEFT JOIN users lu ON lu.id=v.lector"
				" LEFT JOIN uservote uvp ON v.id=uvp.vote AND uvp.vote_for='on'"
				" LEFT JOIN uservote uvc ON v.id=uvc.vote AND uvc.vote_for<>'on'"
				" LEFT JOIN uservote uvm ON v.id=uvm.vote AND uvm.users=" + GetCache()->EscapeValue(user_cur->Id.AsString()) +
				" LEFT JOIN lector ml ON v.id=ml.vote AND ml.users=" + GetCache()->EscapeValue(user_cur->Id.AsString()) +
				" LEFT JOIN lector l ON v.id=l.vote"
				" GROUP BY v.id, v.name, v.descr, u.name, uvm.vote_for, ml.id, u.id, v.level, lu.fullname, v.repdate"
				, q) {
			string author_id = q->AsString("author_id");

			auto elem = ses.NewElem();
			ses.AddChildNode("id", q->AsString(0));
			ses.AddChildNode("name", q->AsString(1));
			ses.AddChildNode("descr", q->AsString(2));
			ses.AddChildNode("author", q->AsString(3));

			if (q->IsNull(4)) {
				ses.AddChildNode("votefor", "on");
				ses.AddChildNode("voteagainst", "on");
				ses.AddChildNode("newvote", "on");
			} else
				ses.AddChildNode("myvote", q->AsString(4));

			bool can_change = false;

			if (user_cur->IsAdmin) {
				can_change = true;
			} else {
				can_change = (author_id == user_cur->Id.AsString());
			}

			ses.AddChildNode("can_edit", B2C(can_change));
			ses.AddChildNode("can_delete", B2C(can_change));

			ses.AddChildNode("pro", q->AsString(5));
			ses.AddChildNode("con", q->AsString(6));
			ses.AddChildNode("mark", q->AsString(7));
			if (q->AsInt(7) < 0)
				ses.AddChildNode("bad", "on");

			ses.AddChildNode("has_lector", B2C(q->AsInt(8) > 0));

			ses.AddChildNode("melector", B2C(!q->IsNull(9)));
			ses.AddChildNode("level", q->AsString(11));

			if (!q->IsNull("lector"))
				ses.AddChildNode("repdate", q->AsString("repdate") + "\n" + q->AsString("lector"));
		}
	}

	virtual void TableFormTune(Session& ses, Cursor &table) const {
		ses.NewSelect("level");
		ses.AddChildNode("msg", THEME_LEVEL_NOVICE);
		ses.AddChildNode("msg", THEME_LEVEL_STANDARD);
		ses.AddChildNode("msg", THEME_LEVEL_PRO);
	}

	virtual void TableDefaults(Session& ses) const {
		ses.NewNode("level", THEME_LEVEL_STANDARD);
	}

	void FillVoters(Session& ses, Cursor &table, const string &field, bool pros) const {
		string voters;
		ForEachQuery(GetCache(),
			"SELECT u.fullname"
			" FROM uservote v"
			" INNER JOIN users u ON u.id=v.users"
			" WHERE v.vote=" + GetCache()->EscapeValue(table->Id.AsString()) +
				" AND v.vote_for" + string(pros ? "='on'" : "<>'on'") +
			" ORDER BY u.fullname"
			, q)
			voters = str::Append(voters, q->AsString(0), ", ");

		ses.NewNode(field.c_str(), voters);
	}

	virtual void TableGet(Session& ses, Cursor &table) const {
		FillVoters(ses, table, "pros", true);
		FillVoters(ses, table, "cons", false);

		auto user_cur = GetCache()->Get<User>(table->Author.AsString());
		ses.NewSelect("author");
		ses.AddChildNode("val", user_cur->FullName);

		string lectors;
		ForEachQuery(GetCache(),
			"SELECT u.fullname"
			" FROM lector l"
			" INNER JOIN users u ON u.id=l.users"
			" WHERE l.vote=" + GetCache()->EscapeValue(table->Id.AsString()) +
			" ORDER BY u.fullname"
			, q)
			lectors = str::Append(lectors, q->AsString(0), ", ");

		ses.NewNode("lectors", lectors);
	}

	virtual void TableSet(Session& ses, Cursor &table) const {
		auto user_cur = GetCache()->Get<User>(ses.auth.ext("uid"));
		if (table->IsNew()) {
			table->Author = user_cur->Id;
			table->Lector.SetNull();
			table->RepDate.SetNull();
		}

		if (!user_cur->IsAdmin) {
			if (table->Author != user_cur->Id)
				throw mgr_err::Error("vote", "edit_not_author").add_param("admins", GetAdmins());
		}
	}

	virtual void TableBeforeDelete(Session& ses, Cursor &table) const {
		auto user_cur = GetCache()->Get<User>(ses.auth.ext("uid"));

		if (!user_cur->IsAdmin) {
			if (table->Author != user_cur->Id)
				throw mgr_err::Error("vote", "delete_not_author").add_param("admins", GetAdmins());
		}
	}

	virtual string ActiveHint(Session &ses, const string &elid, const string &prop_name, const string &value) const {
		auto table = GetCursor();
		if (!FindAndCheck(ses, table, elid))
			throw mgr_err::Value(ELEM_ID, elid);

		string cond;
		if (prop_name == "pro")
			cond = " uv.vote_for='on'";
		else if (prop_name == "con")
			cond = " uv.vote_for<>'on'";
		else
			return "";

		string result;

		ForEachQuery(GetCache(),
			"SELECT u.fullname"
			" FROM vote v"
			" INNER JOIN uservote uv ON v.id=uv.vote AND" + cond +
			" INNER JOIN users u ON u.id=uv.users"
			" WHERE v.id=" + GetCache()->EscapeValue(table->Id.AsString()) +
			" ORDER BY u.fullname"
			, q) {
			result = str::Append(result, q->AsString(0), ", ");
		}

		if (result.empty())
			result = "пока никому";

		return result;
	}

	class SetVote : public GroupAction {
	public:
		SetVote(VoteAction *parent, const string &name, bool vote_value)
		: GroupAction(name, DefaultRights(), parent)
		, m_parent(parent)
		, m_vote_value(vote_value) {}
	private:
		const VoteAction * const m_parent;
		const bool m_vote_value;
		virtual void ProcessOne(Session &ses, const string &elid) const {
			Cursor table = m_parent->GetCursor();
			if (!m_parent->FindAndCheck(ses, table, elid))
				throw mgr_err::Missed("elid", elid);

			auto user_cur = GetCache()->Get<User>(ses.auth.ext("uid"));
			auto uvote_cur = GetCache()->Get<UserVote>();
			if (!uvote_cur->DbFindForUpdate(
				"vote=" + GetCache()->EscapeValue(table->Id.AsString()) +
				" AND users=" + GetCache()->EscapeValue(user_cur->Id.AsString()))) {
				uvote_cur->New();
				uvote_cur->User = user_cur->Id;
				uvote_cur->Vote = table->Id;
			}

			uvote_cur->VoteFor = m_vote_value;
			uvote_cur->Post();
		}
	};

	SetVote m_up;
	SetVote m_down;

	class BeLector : public GroupAction {
	public:
		BeLector(VoteAction *parent)
		: GroupAction("be_lector", DefaultRights(), parent)
		, m_parent(parent) {}
	private:
		const VoteAction * const m_parent;
		virtual void ProcessOne(Session &ses, const string &elid) const {
			Cursor table = m_parent->GetCursor();
			if (!m_parent->FindAndCheck(ses, table, elid))
				throw mgr_err::Missed("elid", elid);

			auto user_cur = GetCache()->Get<User>(ses.auth.ext("uid"));
			auto lector_cur = GetCache()->Get<Lector>();
			if (!lector_cur->DbFindForUpdate(
				"vote=" + GetCache()->EscapeValue(table->Id.AsString()) +
				" AND users=" + GetCache()->EscapeValue(user_cur->Id.AsString()))) {
				lector_cur->New();
				lector_cur->User = user_cur->Id;
				lector_cur->Vote = table->Id;
			}

			lector_cur->Ready = false;
			lector_cur->Post();
		}
	} m_be_lector;

	class NotBeLector : public GroupAction {
	public:
		NotBeLector(VoteAction *parent)
		: GroupAction("not_be_lector", DefaultRights(), parent)
		, m_parent(parent) {}
	private:
		const VoteAction * const m_parent;
		virtual void ProcessOne(Session &ses, const string &elid) const {
			Cursor table = m_parent->GetCursor();
			if (!m_parent->FindAndCheck(ses, table, elid))
				throw mgr_err::Missed("elid", elid);

			auto user_cur = GetCache()->Get<User>(ses.auth.ext("uid"));

			if (table->Lector.AsString() == user_cur->Id.AsString()) {
				table->Lector.SetNull();
				table->RepDate.SetNull();
				table->Post();
			}

			auto lector_cur = GetCache()->Get<Lector>();
			lector_cur->DbDelete(
				"vote=" + GetCache()->EscapeValue(table->Id.AsString()) +
				" AND users=" + GetCache()->EscapeValue(user_cur->Id.AsString()));
		}
	} m_not_be_lector;

	class SetDate : public FormAction {
	public:
		SetDate(VoteAction *parent)
		: FormAction("set_date", DefaultRights(), parent)
		, m_parent(parent) {}
	private:
		const VoteAction * const m_parent;

		virtual void Get(Session &ses, const string &elid) const {
			Cursor vote_cur = m_parent->GetCursor();
			if (!m_parent->FindAndCheck(ses, vote_cur, elid))
				throw mgr_err::Missed("elid", elid);

			string lector;
			if (!vote_cur->Lector.IsNull()) {
				auto user_cur = GetCache()->Get<User>(vote_cur->Lector.AsString());
				lector = user_cur->FullName;
			}
			ses.NewNode("current_lector", lector);

			ses.NewSelect("hour");
			ses.AddChildNode("val", "09");
			ses.AddChildNode("val", "10");
			ses.AddChildNode("val", "11");
			ses.AddChildNode("val", "12");
			ses.AddChildNode("val", "13");
			ses.AddChildNode("val", "14");
			ses.AddChildNode("val", "15");
			ses.AddChildNode("val", "16");
			ses.AddChildNode("val", "17");

			ses.NewSelect("minute");
			ses.AddChildNode("val", "00");
			ses.AddChildNode("val", "30");

			if (vote_cur->RepDate.IsNull() || vote_cur->RepDate.empty()) {
				ses.NewNode("date", mgr_date::DateTime().AsDate());
				ses.NewNode("hour", "15");
				ses.NewNode("minute", "00");
			} else {
				auto ct = vote_cur->RepDate.AsDateTime();
				ses.NewNode("date", ct.AsDate());
				ses.NewNode("hour", str::Str(ct.hour()));
				ses.NewNode("minute", str::Str(ct.minute()));
			}
		}

		virtual void Set(Session &ses, const string &elid) const {
			Cursor vote_cur = m_parent->GetCursor();
			if (!m_parent->FindAndCheck(ses, vote_cur, elid))
				throw mgr_err::Missed("elid", elid);

			auto user_cur = GetCache()->Get<User>(ses.auth.ext("uid"));
			auto lector_cur = GetCache()->Get<Lector>();
			if (!lector_cur->DbFindForUpdate(
				"vote=" + GetCache()->EscapeValue(vote_cur->Id.AsString()) +
				" AND users=" + GetCache()->EscapeValue(user_cur->Id.AsString()))) {
				lector_cur->New();
				lector_cur->User = user_cur->Id;
				lector_cur->Vote = vote_cur->Id;
			}

			lector_cur->Ready = false;
			lector_cur->Post();

			string strdate = ses.Param("date") + " " + ses.Param("hour") + ":" + ses.Param("minute");

			vote_cur->Lector = user_cur->Id;
			vote_cur->RepDate = mgr_date::DateTime(strdate);
			vote_cur->Post();
		}
	} m_set_date;
};

MODULE_INIT(vote, "tables") {
	new VoteAction;
}
} //end of priv_vote namespace
