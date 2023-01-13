#include "lazy_search.h"
#include "search_common.h"

#include "../plugins/plugin.h"

using namespace std;

namespace plugin_lazy_wastar {
static const string DEFAULT_LAZY_BOOST = "1000";

static shared_ptr<SearchEngine> _parse(OptionParser &parser) {
    {
        parser.document_synopsis(
            "(Weighted) A* search (lazy)",
            "Weighted A* is a special case of lazy best first search.");

        parser.add_list_option<shared_ptr<Evaluator>>(
            "evals",
            "evaluators");
        parser.add_list_option<shared_ptr<Evaluator>>(
            "preferred",
            "use preferred operators of these evaluators", "[]");
        parser.add_option<bool>(
            "reopen_closed",
            "reopen closed nodes",
            "true");
        parser.add_option<int>(
            "boost",
            "boost value for preferred operator open lists",
            DEFAULT_LAZY_BOOST);
        parser.add_option<int>("w", "evaluator weight", "1");
        SearchEngine::add_succ_order_options(parser);
        SearchEngine::add_options_to_parser(parser);

        parser.document_note(
            "Open lists",
            "In the general case, it uses an alternation open list "
            "with one queue for each evaluator h that ranks the nodes "
            "by g + w * h. If preferred operator evaluators are used, "
            "it adds for each of the evaluators another such queue that "
            "only inserts nodes that are generated by preferred operators. "
            "In the special case with only one evaluator and no preferred "
            "operator evaluators, it uses a single queue that "
            "is ranked by g + w * h. ");
        parser.document_note(
            "Equivalent statements using general lazy search",
            "\n```\n--evaluator h1=eval1\n"
            "--search lazy_wastar([h1, eval2], w=2, preferred=h1,\n"
            "                     bound=100, boost=500)\n```\n"
            "is equivalent to\n"
            "```\n--evaluator h1=eval1 --heuristic h2=eval2\n"
            "--search lazy(alt([single(sum([g(), weight(h1, 2)])),\n"
            "                   single(sum([g(), weight(h1, 2)]), pref_only=true),\n"
            "                   single(sum([g(), weight(h2, 2)])),\n"
            "                   single(sum([g(), weight(h2, 2)]), pref_only=true)],\n"
            "                  boost=500),\n"
            "              preferred=h1, reopen_closed=true, bound=100)\n```\n"
            "------------------------------------------------------------\n"
            "```\n--search lazy_wastar([eval1, eval2], w=2, bound=100)\n```\n"
            "is equivalent to\n"
            "```\n--search lazy(alt([single(sum([g(), weight(eval1, 2)])),\n"
            "                   single(sum([g(), weight(eval2, 2)]))],\n"
            "                  boost=1000),\n"
            "              reopen_closed=true, bound=100)\n```\n"
            "------------------------------------------------------------\n"
            "```\n--search lazy_wastar([eval1, eval2], bound=100, boost=0)\n```\n"
            "is equivalent to\n"
            "```\n--search lazy(alt([single(sum([g(), eval1])),\n"
            "                   single(sum([g(), eval2]))])\n"
            "              reopen_closed=true, bound=100)\n```\n"
            "------------------------------------------------------------\n"
            "```\n--search lazy_wastar(eval1, w=2)\n```\n"
            "is equivalent to\n"
            "```\n--search lazy(single(sum([g(), weight(eval1, 2)])), reopen_closed=true)\n```\n",
            true);
    }
    Options opts = parser.parse();

    opts.verify_list_non_empty<shared_ptr<Evaluator>>("evals");

    shared_ptr<lazy_search::LazySearch> engine;
    if (!parser.dry_run()) {
        opts.set("open", search_common::create_wastar_open_list_factory(opts));
        engine = make_shared<lazy_search::LazySearch>(opts);
        // TODO: The following two lines look fishy. See similar comment in _parse.
        vector<shared_ptr<Evaluator>> preferred_list = opts.get_list<shared_ptr<Evaluator>>("preferred");
        engine->set_preferred_operator_evaluators(preferred_list);
    }
    return engine;
}

static Plugin<SearchEngine> _plugin("lazy_wastar", _parse);
}
