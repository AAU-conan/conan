#ifndef SYMBOLIC_MERGE_BDDS_H
#define SYMBOLIC_MERGE_BDDS_H

#include <vector>
#include "cuddObj.hh"
#include "../utils/countdown_timer.h"
#include "bdd_manager.h"


namespace symbolic {
    BDD mergeAndBDD(const BDD &bdd, const BDD &bdd2, int maxSize);

    BDD mergeOrBDD(const BDD &bdd, const BDD &bdd2, int maxSize);

    ADD mergeSumADD(const ADD &add, const ADD &add2, int);

    ADD mergeMaxADD(const ADD &add, const ADD &add2, int);


    template<typename T>
    int nodeCount(std::shared_ptr<T> t) {
        return t->nodeCount();
    }

    template<typename T>
    int nodeCount(T t) {
        return t.nodeCount();
    }

    template<class T, class FunctionMerge>
    void mergeAux(std::vector<T> &elems, FunctionMerge f, utils::Duration maxTime, long maxSize) {
        std::vector<T> result;
        if (maxSize <= 1 || elems.size() <= 1) {
            return;
        }
        utils::CountdownTimer merge_timer(maxTime);
        //  cout << "Merging " << elems.size() << ", maxSize: " << maxSize << endl;

        //Merge Elements
        std::vector<T> aux;
        while (elems.size() > 1 && !merge_timer.is_expired()) {
            if (elems.size() % 2 == 1) { //Ensure an even number
                int last = elems.size() - 1;
                try {
                    T res = f(elems[last - 1], elems[last], maxSize);
                    elems[last - 1] = std::move(res);
                } catch (BDDError e) {
                    result.push_back(elems[last]);
                }
                elems.erase(elems.end() - 1);
            }
            //    cout << "Iteration: " << elems.size() << endl;
            for (size_t i = 1; i < elems.size(); i += 2) {
                try {
                    T res = f(elems[i - 1], elems[i], maxSize);
                    aux.push_back(res);
                } catch (BDDError e) {
                    if (nodeCount(elems[i]) < nodeCount(elems[i - 1])) {
                        result.push_back(elems[i - 1]);
                        aux.push_back(elems[i]);
                    } else {
                        result.push_back(elems[i]);
                        aux.push_back(elems[i - 1]);
                    }
                }
            }
            aux.swap(elems);
            std::vector<T>().swap(aux);
        }
        //  cout << "Finished: " << elems.size() << endl;
        if (!elems.empty()) {
            result.insert(result.end(), elems.begin(), elems.end());
        }
        result.swap(elems);
        //Add all the elements remaining in aux
        if (!aux.empty()) {
            elems.insert(elems.end(), aux.begin(), aux.end());
        }
        /*for(int i = 0; i < aux.size(); i++){
          elems.push_back(aux[i]);
          }*/

        //  cout << "Merged to " << elems.size() << ". Took "<< merge_timer << " seconds" << endl;
    }

/*
 * Method that merges some elements,
 * Relays on several methods: T, int T.size() and bool T.merge(T, maxSize)
 */
    template<class T, class FunctionMerge>
    void merge(BDDManager *mgr, std::vector<T> &elems, FunctionMerge f, utils::Duration maxTime, long maxSize) {
        mgr->perform_operation_with_time_limit(maxTime, [&]() {
            mergeAux(elems, f, maxTime, maxSize);
        });
    }

    /*
     * Method that merges some elements,
     * Relays on several methods: T, int T.size() and bool T.merge(T, maxSize)
     */
    template<class T, class FunctionMerge>
    void merge(std::vector<T> &elems, FunctionMerge f, long maxSize) {
        mergeAux(elems, f, 0, maxSize);
    }
}


#endif
