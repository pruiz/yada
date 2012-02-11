
################################################################################
################################################################################

# yada ebuild file 
# 
# $Header: $

inherit eutils

################################################################################

DESCRIPTION="yet another database abstraction"
HOMEPAGE="http://oss.devit.com/yada/"
SRC_URI="ftp://ftp.devit.com/yada/${P}.tar.bz2"
LICENSE="GPL-3"

SLOT="0"
KEYWORDS="~x86"
IUSE="mysql oracle postgres sqlite3 yadac"

################################################################################

COMMONDEPS="
	mysql? ( virtual/mysql )
	oracle? ( dev-db/oracle-instantclient-basic )
	postgres? ( dev-db/postgresq )
	sqlite3? ( >=dev-db/sqlite-3.0.0 )
"

DEPEND="
	${COMMONDEPS}
	yadac? ( >=sys-devel/flex-2.5.33 )
	yadac? ( >=sys-devel/bison-2.3 )
"

RDEPEND="${COMMONDEPS}"

################################################################################

src_compile() {
	econf \
	  $(use_with mysql) \
	  $(use_with oracle) \
	  $(use_with postgres) \
	  $(use_with sqlite3) \
	  $(use_enable yadac) \
	|| die "econf failed"

	emake || die "emake failed"
}

################################################################################

src_install() {
	emake DESTDIR="${D}" install || die "emake install failed"
}

################################################################################
################################################################################

