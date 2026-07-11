#include "naui/core/action.h"
#include "test.h"

#include <stdlib.h>
#include <string.h>

typedef struct
{
	int *counter;
	int amount;
}
CounterData;

static bool counter_execute(void *data)
{
	CounterData *d = data;
	*d->counter += d->amount;
	return true;
}

static bool counter_undo(void *data)
{
	CounterData *d = data;
	*d->counter -= d->amount;
	return true;
}

static CounterData *make_counter_data(int *counter, int amount)
{
	CounterData *data = malloc(sizeof(CounterData));
	*data = (CounterData){ counter, amount };
	return data;
}

static bool always_fails_execute(void *data)
{
	(void)data;
	return false;
}

static bool always_fails_undo(void *data)
{
	(void)data;
	return false;
}

typedef struct
{
	int *destroy_count;
}
DestroyTrackedData;

static bool tracked_execute(void *data) { (void)data; return true; }
static bool tracked_undo(void *data)	{ (void)data; return true; }

static void tracked_destroy(void *data)
{
	DestroyTrackedData *d = data;
	(*d->destroy_count)++;
}

static void register_fixtures(void)
{
	naui_register_action("counter_add", {
		.execute = counter_execute,
		.undo = counter_undo
	});

	naui_register_action("always_fails_execute", {
		.execute = always_fails_execute,
		.undo = counter_undo // never reached if execute always fails
	});

	naui_register_action("always_fails_undo", {
		.execute = counter_execute,
		.undo = always_fails_undo
	});

	naui_register_action("destroy_tracked", {
		.execute = tracked_execute,
		.undo = tracked_undo,
		.destroy = tracked_destroy
	});
}

static void reset_action_system(void)
{
	naui_action_clear_history();
	naui_action_set_history_capacity(8);
}

static void test_execute_unregistered_name_fails(void)
{
	TEST_BEGIN("execute: unregistered name returns false, ownership stays with caller");
	reset_action_system();

	CounterData *data = make_counter_data(&(int){0}, 5);
	bool result = naui_action_execute("does_not_exist", data);

	ASSERT(result == false);
	ASSERT(naui_action_can_undo() == false);

	free(data); // caller still owns it
	TEST_END();
}

static void test_execute_success_runs_and_pushes(void)
{
	TEST_BEGIN("execute: success runs execute() and pushes to undo history");
	reset_action_system();

	int counter = 0;
	naui_action_execute("counter_add", make_counter_data(&counter, 5));

	ASSERT(counter == 5);
	ASSERT(naui_action_can_undo() == true);
	ASSERT_STR_EQ(naui_action_undo_name(), "counter_add");
	TEST_END();
}

static void test_execute_failure_does_not_push(void)
{
	TEST_BEGIN("execute: failed execute() does not push to history");
	reset_action_system();

	CounterData *data = make_counter_data(&(int){0}, 5);
	bool result = naui_action_execute("always_fails_execute", data);

	ASSERT(result == false);
	ASSERT(naui_action_can_undo() == false);
	ASSERT_NULL(naui_action_undo_name());

	free(data); // Manual cleanup, action system never took ownership of data
	TEST_END();
}

static void test_undo_reverts_and_moves_to_redo(void)
{
	TEST_BEGIN("undo: reverts the effect and moves the entry to redo");
	reset_action_system();

	int counter = 0;
	naui_action_execute("counter_add", make_counter_data(&counter, 5));
	ASSERT(counter == 5);

	bool result = naui_action_undo();
	ASSERT(result == true);
	ASSERT(counter == 0);
	ASSERT(naui_action_can_undo() == false);
	ASSERT(naui_action_can_redo() == true);
	ASSERT_STR_EQ(naui_action_redo_name(), "counter_add");
	TEST_END();
}

static void test_redo_reapplies_and_moves_back_to_undo(void)
{
	TEST_BEGIN("redo: reapplies the effect and moves the entry back to undo");
	reset_action_system();

	int counter = 0;
	naui_action_execute("counter_add", make_counter_data(&counter, 5));
	naui_action_undo();

	bool result = naui_action_redo();
	ASSERT(result == true);
	ASSERT(counter == 5);
	ASSERT(naui_action_can_redo() == false);
	ASSERT(naui_action_can_undo() == true);
	ASSERT_STR_EQ(naui_action_undo_name(), "counter_add");
	TEST_END();
}

static void test_undo_with_empty_history_returns_false(void)
{
	TEST_BEGIN("undo: empty history returns false and changes nothing");
	reset_action_system();

	ASSERT(naui_action_undo() == false);
	TEST_END();
}

static void test_redo_with_empty_history_returns_false(void)
{
	TEST_BEGIN("redo: empty history returns false and changes nothing");
	reset_action_system();

	ASSERT(naui_action_redo() == false);
	TEST_END();
}

static void test_new_execute_clears_redo_stack(void)
{
	TEST_BEGIN("execute: a new commit clears any pending redo history");
	reset_action_system();

	int counter = 0;
	naui_action_execute("counter_add", make_counter_data(&counter, 5));
	naui_action_undo();
	ASSERT(naui_action_can_redo() == true);

	naui_action_execute("counter_add", make_counter_data(&counter, 3));
	ASSERT(naui_action_can_redo() == false);
	ASSERT_NULL(naui_action_redo_name());
	TEST_END();
}

static void test_failed_undo_leaves_entry_in_place(void)
{
	TEST_BEGIN("undo: a failing undo() leaves the entry on the undo stack, not moved");
	reset_action_system();

	int counter = 0;
	naui_action_execute("always_fails_undo", make_counter_data(&counter, 5));
	ASSERT(counter == 5);

	bool result = naui_action_undo();
	ASSERT(result == false);
	ASSERT(counter == 5); // Nothing should change since undo failed
	ASSERT(naui_action_can_undo() == true); // Still there, not popped
	ASSERT(naui_action_can_redo() == false); // Didn't move to redo
	ASSERT_STR_EQ(naui_action_undo_name(), "always_fails_undo");
	TEST_END();
}

static void test_multiple_undo_redo_order(void)
{
	TEST_BEGIN("undo/redo: multiple entries unwind and reapply in the correct order");
	reset_action_system();

	int counter = 0;
	naui_action_execute("counter_add", make_counter_data(&counter, 1));
	naui_action_execute("counter_add", make_counter_data(&counter, 2));
	naui_action_execute("counter_add", make_counter_data(&counter, 4));

	naui_action_undo();
	ASSERT(counter == 3);
	naui_action_undo();
	ASSERT(counter == 1);

	naui_action_redo();
	ASSERT(counter == 3);

	naui_action_undo();
	naui_action_undo();
	ASSERT(counter == 0);
	ASSERT(naui_action_can_undo() == false);
	TEST_END();
}

static void test_capacity_evicts_oldest_on_overflow(void)
{
	TEST_BEGIN("capacity: pushing past capacity evicts the oldest entry, not the newest");
	reset_action_system();
	naui_action_set_history_capacity(3);

	int counter = 0;
	naui_action_execute("counter_add", make_counter_data(&counter, 1));
	naui_action_execute("counter_add", make_counter_data(&counter, 2));
	naui_action_execute("counter_add", make_counter_data(&counter, 3));
	naui_action_execute("counter_add", make_counter_data(&counter, 4));

	naui_action_undo();
	naui_action_undo();
	naui_action_undo();
	ASSERT(counter == 1);
	ASSERT(naui_action_can_undo() == false);
	TEST_END();
}

static void test_capacity_shrink_evicts_and_destroys_oldest(void)
{
	TEST_BEGIN("capacity: shrinking below current count evicts + destroys the oldest entries immediately");
	reset_action_system();
	naui_action_set_history_capacity(5);

	int destroy_count = 0;
	for(int i = 0; i < 5; i++)
	{
		DestroyTrackedData *data = malloc(sizeof(DestroyTrackedData));
		*data = (DestroyTrackedData){ &destroy_count };
		naui_action_execute("destroy_tracked", data);
	}

	naui_action_set_history_capacity(2);

	ASSERT(destroy_count == 3);
	ASSERT(naui_action_get_history_capacity() == 2);
	TEST_END();
}

static void test_capacity_preserves_remaining_order_after_resize(void)
{
	TEST_BEGIN("capacity: entries kept after a resize still undo in the correct order");
	reset_action_system();
	naui_action_set_history_capacity(5);

	int counter = 0;
	naui_action_execute("counter_add", make_counter_data(&counter, 1));
	naui_action_execute("counter_add", make_counter_data(&counter, 2));
	naui_action_execute("counter_add", make_counter_data(&counter, 3));
	ASSERT(counter == 6);

	naui_action_set_history_capacity(10);
	naui_action_undo();
	ASSERT(counter == 3);
	naui_action_undo();
	ASSERT(counter == 1);
	TEST_END();
}

static void test_clear_history_destroys_everything(void)
{
	TEST_BEGIN("clear_history: destroys and empties both undo and redo stacks");
	reset_action_system();

	int destroy_count = 0;
	DestroyTrackedData *d1 = malloc(sizeof(DestroyTrackedData)); *d1 = (DestroyTrackedData){ &destroy_count };
	DestroyTrackedData *d2 = malloc(sizeof(DestroyTrackedData)); *d2 = (DestroyTrackedData){ &destroy_count };
	naui_action_execute("destroy_tracked", d1);
	naui_action_execute("destroy_tracked", d2);
	naui_action_undo();

	naui_action_clear_history();

	ASSERT(destroy_count == 2);
	ASSERT(naui_action_can_undo() == false);
	ASSERT(naui_action_can_redo() == false);
	TEST_END();
}

static void test_group_collapses_to_single_history_entry(void)
{
	TEST_BEGIN("group: multiple executes inside a group become one undo/redo step");
	reset_action_system();

	int counter = 0;
	naui_action_group_start("Add Three");
	naui_action_execute("counter_add", make_counter_data(&counter, 1));
	naui_action_execute("counter_add", make_counter_data(&counter, 2));
	naui_action_execute("counter_add", make_counter_data(&counter, 3));
	bool committed = naui_action_group_end();

	ASSERT(committed == true);
	ASSERT(counter == 6);
	ASSERT_STR_EQ(naui_action_undo_name(), "Add Three");
	naui_action_undo();

	ASSERT(counter == 0);
	ASSERT(naui_action_can_undo() == false);
	TEST_END();
}

static void test_group_redo_replays_forward_order(void)
{
	TEST_BEGIN("group: redo replays the whole group forward, in original order");
	reset_action_system();

	int counter = 0;
	naui_action_group_start("Add Three");
	naui_action_execute("counter_add", make_counter_data(&counter, 1));
	naui_action_execute("counter_add", make_counter_data(&counter, 2));
	naui_action_execute("counter_add", make_counter_data(&counter, 3));
	naui_action_group_end();

	naui_action_undo();
	ASSERT(counter == 0);

	bool result = naui_action_redo();

	ASSERT(result == true);
	ASSERT(counter == 6);
	TEST_END();
}

static void test_empty_group_is_not_pushed(void)
{
	TEST_BEGIN("group: a group where every action failed is discarded, not pushed as a no-op");
	reset_action_system();

	naui_action_group_start("Should Not Exist");
	CounterData *data = make_counter_data(&(int){0}, 5);
	bool inner_result = naui_action_execute("always_fails_execute", data);
	bool committed = naui_action_group_end();

	ASSERT(inner_result == false);
	ASSERT(committed == false);
	ASSERT(naui_action_can_undo() == false);
	ASSERT_NULL(naui_action_undo_name());

	free(data);
	TEST_END();
}

static void test_partial_failure_group_only_contains_successes(void)
{
	TEST_BEGIN("group: only the actions that actually succeeded end up in the committed group");
	reset_action_system();

	int counter = 0;
	naui_action_group_start("Partial");
	naui_action_execute("counter_add", make_counter_data(&counter, 5));
	naui_action_execute("always_fails_execute", make_counter_data(&counter, 999));
	naui_action_execute("counter_add", make_counter_data(&counter, 2));
	naui_action_group_end();

	ASSERT(counter == 7);

	naui_action_undo();

	ASSERT(counter == 0);
	TEST_END();
}

static void test_group_costs_exactly_one_history_slot(void)
{
	TEST_BEGIN("group: a group counts as exactly one entry against history capacity");
	reset_action_system();
	naui_action_set_history_capacity(2);

	int counter = 0;
	naui_action_group_start("Group A");
	naui_action_execute("counter_add", make_counter_data(&counter, 1));
	naui_action_execute("counter_add", make_counter_data(&counter, 1));
	naui_action_execute("counter_add", make_counter_data(&counter, 1));
	naui_action_group_end();

	naui_action_execute("counter_add", make_counter_data(&counter, 10));
	ASSERT(counter == 13);

	naui_action_undo();
	ASSERT(counter == 3);
	naui_action_undo(); // Undoes the whole group at once
	ASSERT(counter == 0);
	ASSERT(naui_action_can_undo() == false); // Exactly two slots were used, not four
	TEST_END();
}

int action_test(void)
{
	register_fixtures();

	test_execute_unregistered_name_fails();
	test_execute_success_runs_and_pushes();
	test_execute_failure_does_not_push();

	test_undo_reverts_and_moves_to_redo();
	test_redo_reapplies_and_moves_back_to_undo();
	test_undo_with_empty_history_returns_false();
	test_redo_with_empty_history_returns_false();
	test_new_execute_clears_redo_stack();
	test_failed_undo_leaves_entry_in_place();
	test_multiple_undo_redo_order();

	test_capacity_evicts_oldest_on_overflow();
	test_capacity_shrink_evicts_and_destroys_oldest();
	test_capacity_preserves_remaining_order_after_resize();
	test_clear_history_destroys_everything();

	test_group_collapses_to_single_history_entry();
	test_group_redo_replays_forward_order();
	test_empty_group_is_not_pushed();
	test_partial_failure_group_only_contains_successes();
	test_group_costs_exactly_one_history_slot();

	test_conclusion();
	return 0;
}