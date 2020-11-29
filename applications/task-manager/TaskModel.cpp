#include "task-manager/TaskModel.h"
#include <libsystem/process/Process.h>

enum Column
{
    COLUMN_ID,
    COLUMN_NAME,
    COLUMN_STATE,
    COLUMN_CPU,
    COLUMN_RAM,

    __COLUMN_COUNT,
};

int TaskModel::rows()
{
    return _data.length();
}

int TaskModel::columns()
{
    return __COLUMN_COUNT;
}

String TaskModel::header(int column)
{
    switch (column)
    {
    case COLUMN_ID:
        return "ID";

    case COLUMN_NAME:
        return "Name";

    case COLUMN_STATE:
        return "State";

    case COLUMN_CPU:
        return "CPU%";

    case COLUMN_RAM:
        return "RAM(Kio)";

    default:
        ASSERT_NOT_REACHED();
    }
}

Variant TaskModel::data(int row, int column)
{
    auto &task = _data.get((size_t)row);

    switch (column)
    {
    case COLUMN_ID:
    {
        Variant value = task.get("id").as_integer();

        if (task.get("user").is(json::TRUE))
        {
            return value.with_icon(Icon::get("account"));
        }
        else
        {
            return value.with_icon(Icon::get("cog"));
        }
    }

    case COLUMN_NAME:
        return task.get("name").as_string();

    case COLUMN_STATE:
        return task.get("state").as_string();

    case COLUMN_CPU:
        return Variant("%2d%%", task.get("cpu").as_integer());

    case COLUMN_RAM:
        return Variant("%5d Kio", task.get("ram").as_integer() / 1024);

    default:
        ASSERT_NOT_REACHED();
    }
}

void TaskModel::update()
{
    _data = json::parse_file("/System/processes");
    did_update();
}

static String greedy(json::Value &data, const char *field)
{
    size_t most_greedy_index = 0;
    int most_greedy_value = 0;

    for (size_t i = 0; i < data.length(); i++)
    {
        auto &task = data.get(i);

        auto value = task.get(field).as_integer();

        if (value > most_greedy_value)
        {
            most_greedy_index = i;
            most_greedy_value = value;
        }
    }

    return data
        .get(most_greedy_index)
        .get("name")
        .as_string();
}

String TaskModel::ram_greedy()
{
    return greedy(_data, "ram");
}

String TaskModel::cpu_greedy()
{
    return greedy(_data, "cpu");
}

void TaskModel::kill_task(int row)
{
    process_cancel(data(row, COLUMN_ID).as_int());
}
