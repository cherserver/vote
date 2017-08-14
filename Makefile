VERSION = 0.0.1
MGR = vote
LANGS = ru

LIB += vote
vote_SOURCES = db_user.cpp tables.cpp db_auth.cpp vote.cpp
vote_LDADD = -lbase

include ../isp.mk
