#include <test_helpers.hxx>

using namespace PGSTD;
using namespace pqxx;

namespace
{
void test_forward_sql_cursor(connection_base &, transaction_base &trans)
{
  // Plain owned, scoped, forward-only read-only cursor.
  internal::sql_cursor forward(
	trans,
	pqxx::test::select_series(trans.conn(), 1, 4),
	"forward",
	cursor_base::forward_only,
	cursor_base::read_only,
	cursor_base::owned,
	false);

  PQXX_CHECK_EQUAL(forward.pos(), 0, "Wrong initial position");
  PQXX_CHECK_EQUAL(forward.endpos(), -1, "Wrong initial endpos()");

  result initial_row = forward.fetch_current_row();
  PQXX_CHECK_EQUAL(initial_row.size(), 0, "Starting row not empty");

  result empty_result = forward.empty_result();
  PQXX_CHECK_EQUAL(empty_result.size(), 0, "Empty result not empty");

  cursor_base::difference_type displacement = 0;
  result one = forward.fetch(1, displacement);
  PQXX_CHECK_EQUAL(one.size(), 1, "Fetched wrong number of rows");
  PQXX_CHECK_EQUAL(one[0][0].as<string>(), "1", "Unexpected result");
  PQXX_CHECK_EQUAL(displacement, 1, "Wrong displacement");
  PQXX_CHECK_EQUAL(forward.pos(), 1, "In wrong position");

  cursor_base::difference_type offset = forward.move(1, displacement);
  PQXX_CHECK_EQUAL(offset, 1, "Unexpected offset from move()");
  PQXX_CHECK_EQUAL(displacement, 1, "Unexpected displacement after move()");
  PQXX_CHECK_EQUAL(forward.pos(), 2, "Wrong position after move()");
  PQXX_CHECK_EQUAL(forward.endpos(), -1, "endpos() unexpectedly set");

  result row = forward.fetch(0, displacement);
  PQXX_CHECK_EQUAL(row.size(), 0, "fetch(0, displacement) returns rows");
  PQXX_CHECK_EQUAL(displacement, 0, "Unexpected displacement after fetch(0)");
  PQXX_CHECK_EQUAL(forward.pos(), 2, "fetch(0, displacement) affected pos()");

  row = forward.fetch(0);
  PQXX_CHECK_EQUAL(row.size(), 0, "fetch(0) fetched wrong number of rows");
  PQXX_CHECK_EQUAL(forward.pos(), 2, "fetch(0) moved cursor");
  PQXX_CHECK_EQUAL(forward.pos(), 2, "fetch(0) affected pos()");

  offset = forward.move(1);
  PQXX_CHECK_EQUAL(offset, 1, "move(1) returned unexpected value");
  PQXX_CHECK_EQUAL(forward.pos(), 3, "move(1) after fetch(0) broke");

  row = forward.fetch(1);
  PQXX_CHECK_EQUAL(row.size(), 1, "fetch(1) returned wrong number of rows");
  PQXX_CHECK_EQUAL(forward.pos(), 4, "fetch(1) results in bad pos()");
  PQXX_CHECK_EQUAL(row[0][0].as<string>(), "4", "pos() is lying");

  empty_result = forward.fetch(1, displacement);
  PQXX_CHECK_EQUAL(empty_result.size(), 0, "Got rows at end of cursor");
  PQXX_CHECK_EQUAL(forward.pos(), 5, "Not at one-past-end position");
  PQXX_CHECK_EQUAL(forward.endpos(), 5, "Failed to notice end position");
  PQXX_CHECK_EQUAL(displacement, 1, "Wrong displacement at end position");

  offset = forward.move(5, displacement);
  PQXX_CHECK_EQUAL(offset, 0, "move() lied at end of result set");
  PQXX_CHECK_EQUAL(forward.pos(), 5, "pos() is beyond end");
  PQXX_CHECK_EQUAL(forward.endpos(), 5, "endpos() changed after end position");
  PQXX_CHECK_EQUAL(displacement, 0, "Wrong displacement after end position");

  // Move through entire result set at once.
  internal::sql_cursor forward2(
	trans,
	pqxx::test::select_series(trans.conn(), 1, 4),
	"forward",
	cursor_base::forward_only,
	cursor_base::read_only,
	cursor_base::owned,
	false);

  // Move through entire result set at once.
  offset = forward2.move(cursor_base::all(), displacement);
  PQXX_CHECK_EQUAL(offset, 4, "Unexpected number of rows in result set");
  PQXX_CHECK_EQUAL(displacement, 5, "displacement != rows+1");
  PQXX_CHECK_EQUAL(forward2.pos(), 5, "Bad pos() after skipping all rows");
  PQXX_CHECK_EQUAL(forward2.endpos(), 5, "Bad endpos() after skipping");

  internal::sql_cursor forward3(
	trans,
	pqxx::test::select_series(trans.conn(), 1, 4),
	"forward",
	cursor_base::forward_only,
	cursor_base::read_only,
	cursor_base::owned,
	false);

  // Fetch entire result set at once.
  result rows = forward3.fetch(cursor_base::all(), displacement);
  PQXX_CHECK_EQUAL(rows.size(), 4, "Unexpected number of rows in result set");
  PQXX_CHECK_EQUAL(displacement, 5, "displacement != rows+1");
  PQXX_CHECK_EQUAL(forward3.pos(), 5, "Bad pos() after fetching all rows");
  PQXX_CHECK_EQUAL(forward3.endpos(), 5, "Bad endpos() after fetching");

  internal::sql_cursor forward_empty(
	trans,
	pqxx::test::select_series(trans.conn(), 0, -1),
	"forward_empty",
	cursor_base::forward_only,
	cursor_base::read_only,
	cursor_base::owned,
	false);

  offset = forward_empty.move(3, displacement);
  PQXX_CHECK_EQUAL(forward_empty.pos(), 1, "Bad pos() at end of result");
  PQXX_CHECK_EQUAL(forward_empty.endpos(), 1, "Bad endpos() in empty result");
  PQXX_CHECK_EQUAL(displacement, 1, "Bad displacement in empty result");
  PQXX_CHECK_EQUAL(offset, 0, "move() in empty result counted rows");
}

void test_scroll_sql_cursor(connection_base &, transaction_base &trans)
{
  internal::sql_cursor scroll(
	trans,
	pqxx::test::select_series(trans.conn(), 1, 10),
	"scroll",
	cursor_base::random_access,
	cursor_base::read_only,
	cursor_base::owned,
	false);

  PQXX_CHECK_EQUAL(scroll.pos(), 0, "Scroll cursor's initial pos() is wrong");
  PQXX_CHECK_EQUAL(scroll.endpos(), -1, "New scroll cursor has endpos() set");

  result rows = scroll.fetch(cursor_base::next());
  PQXX_CHECK_EQUAL(rows.size(), 1, "Scroll cursor is broken");
  PQXX_CHECK_EQUAL(scroll.pos(), 1, "Scroll cursor's pos() is broken");
  PQXX_CHECK_EQUAL(scroll.endpos(), -1, "endpos() set prematurely");

  // Turn cursor around.  This is where we begin to feel SQL cursors' semantics:
  // we pre-decrement, ending up on the position in front of the first row and
  // returning no rows.
  rows = scroll.fetch(cursor_base::prior());
  PQXX_CHECK_EQUAL(rows.empty(), true, "Turning around on fetch() broke");
  PQXX_CHECK_EQUAL(scroll.pos(), 0, "pos() is not back at zero");
  PQXX_CHECK_EQUAL(scroll.endpos(), -1, "endpos() set on wrong side of result");

  // Bounce off the left-hand side of the result set.  Can't move before the
  // starting position.
  cursor_base::difference_type offset = 0, displacement = 0;
  offset = scroll.move(-3, displacement);
  PQXX_CHECK_EQUAL(offset, 0, "Rows found before beginning");
  PQXX_CHECK_EQUAL(displacement, 0, "Failed to bounce off beginning");
  PQXX_CHECK_EQUAL(scroll.pos(), 0, "pos() moved back from zero");
  PQXX_CHECK_EQUAL(scroll.endpos(), -1, "endpos() set on left-side bounce");

  // Try bouncing off the left-hand side a little harder.  Take 4 paces away
  // from the boundary and run into it.
  offset = scroll.move(4, displacement);
  PQXX_CHECK_EQUAL(offset, 4, "Offset mismatch");
  PQXX_CHECK_EQUAL(displacement, 4, "Displacement mismatch");
  PQXX_CHECK_EQUAL(scroll.pos(), 4, "Position mismatch");
  PQXX_CHECK_EQUAL(scroll.endpos(), -1, "endpos() set at weird time");

  offset = scroll.move(-10, displacement);
  PQXX_CHECK_EQUAL(offset, 3, "Offset mismatch");
  PQXX_CHECK_EQUAL(displacement, -4, "Displacement mismatch");
  PQXX_CHECK_EQUAL(scroll.pos(), 0, "Hard bounce failed");
  PQXX_CHECK_EQUAL(scroll.endpos(), -1, "endpos() set during hard bounce");

  rows = scroll.fetch(3);
  PQXX_CHECK_EQUAL(scroll.pos(), 3, "Bad pos()");
  PQXX_CHECK_EQUAL(rows.size(), 3, "Wrong number of rows");
  PQXX_CHECK_EQUAL(rows[2][0].as<int>(), 3, "pos() does not match data");
  rows = scroll.fetch(-1);
  PQXX_CHECK_EQUAL(scroll.pos(), 2, "Bad pos()");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 2, "pos() does not match data");

  rows = scroll.fetch(1);
  PQXX_CHECK_EQUAL(scroll.pos(), 3, "Bad pos() after inverse turnaround");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 3, "Data position mismatch");
}

void test_adopted_sql_cursor(connection_base &conn, transaction_base &trans)
{
  trans.exec(
	"DECLARE adopted SCROLL CURSOR FOR " +
	pqxx::test::select_series(conn, 1, 3));
  internal::sql_cursor adopted(trans, "adopted", cursor_base::owned);
  PQXX_CHECK_EQUAL(adopted.pos(), -1, "Adopted cursor has known pos()");
  PQXX_CHECK_EQUAL(adopted.endpos(), -1, "Adopted cursor has known endpos()");

  cursor_base::difference_type displacement = 0;
  result rows = adopted.fetch(cursor_base::all(), displacement);
  PQXX_CHECK_EQUAL(rows.size(), 3, "Wrong number of rows in result");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 1, "Wrong result data");
  PQXX_CHECK_EQUAL(rows[2][0].as<int>(), 3, "Wrong result data");
  PQXX_CHECK_EQUAL(displacement, 4, "Wrong displacement");
  PQXX_CHECK_EQUAL(adopted.pos(), -1, "End-of-result set pos() on adopted cur");
  PQXX_CHECK_EQUAL(adopted.endpos(), -1, "endpos() set too early");

  rows = adopted.fetch(cursor_base::backward_all(), displacement);
  PQXX_CHECK_EQUAL(rows.size(), 3, "Wrong number of rows in result");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 3, "Wrong result data");
  PQXX_CHECK_EQUAL(rows[2][0].as<int>(), 1, "Wrong result data");
  PQXX_CHECK_EQUAL(displacement, -4, "Wrong displacement");
  PQXX_CHECK_EQUAL(adopted.pos(), 0, "Failed to recognize starting position");
  PQXX_CHECK_EQUAL(adopted.endpos(), -1, "endpos() set too early");

  cursor_base::difference_type offset = adopted.move(cursor_base::all());
  PQXX_CHECK_EQUAL(offset, 3, "Unexpected move() offset");
  PQXX_CHECK_EQUAL(adopted.pos(), 4, "Bad position on adopted cursor");
  PQXX_CHECK_EQUAL(adopted.endpos(), 4, "endpos() not set properly");

  // Owned adopted cursors are cleaned up on destruction.
  trans.exec(
	"DECLARE adopted2 CURSOR FOR " +
	pqxx::test::select_series(conn, 1, 3));
  {
    internal::sql_cursor(trans, "adopted2", cursor_base::owned);
  }
  PQXX_CHECK_THROWS(
	trans.exec("MOVE 1 IN adopted2"),
	 sql_error,
	 "Owned adopted cursor not cleaned up");

  trans.abort();

  work trans2(conn, "trans2");
  trans2.exec(
	"DECLARE adopted3 CURSOR FOR " +
	pqxx::test::select_series(conn, 1, 3));
  {
    internal::sql_cursor(trans2, "adopted3", cursor_base::loose);
  }
  trans2.exec("MOVE 1 IN adopted3");
}

void test_hold_cursor(connection_base &conn, transaction_base &trans)
{
  // "With hold" cursor is kept after commit.
  internal::sql_cursor with_hold(
	trans,
	pqxx::test::select_series(conn, 1, 3),
	"hold_cursor",
	cursor_base::forward_only,
	cursor_base::read_only,
	cursor_base::owned,
	true);
  trans.commit();
  work trans2(conn, "trans2");
  result rows = with_hold.fetch(1);
  PQXX_CHECK_EQUAL(rows.size(), 1, "Did not get 1 row from with-hold cursor");

  // Cursor without hold is closed on commit.
  internal::sql_cursor no_hold(
	trans2,
	pqxx::test::select_series(conn, 1, 3),
	"no_hold_cursor",
	cursor_base::forward_only,
	cursor_base::read_only,
	cursor_base::owned,
	false);
  trans2.commit();
  work trans3(conn, "trans3");
  PQXX_CHECK_THROWS(no_hold.fetch(1), sql_error, "Cursor not closed on commit");
}

} // namespace

int main()
{
  int x;

  test::TestCase<> test1("forward_sql_cursor", test_forward_sql_cursor);
  x = test::pqxxtest(test1);
  if (x != 0) return x;

  test::TestCase<> test2("scroll_sql_cursor", test_scroll_sql_cursor);
  x = test::pqxxtest(test2);
  if (x != 0) return x;

  test::TestCase<> test3("adopted_sql_cursor", test_adopted_sql_cursor);
  x = test::pqxxtest(test3);
  if (x != 0) return x;

  test::TestCase<> test4("hold_cursor", test_hold_cursor);
  x = test::pqxxtest(test4);
  if (x != 0) return x;

  return 0;
}
