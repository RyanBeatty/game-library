#ifndef UNITY_BUILD
#include <stdint.h>

#include "entity.c"
#include "level.c"
#include "raylib.h"
#endif

typedef struct Transaction {
    Entity* entities;
} Transaction;

static Transaction* Txns = NULL;
static Transaction* ActiveTxn = NULL;

static void TransactionsReset() {
    Txns = NULL;
    ActiveTxn = NULL;
}

static void TransactionBegin() {
    Transaction* txn = arraddnptr(Txns, 1);
    ActiveTxn = txn;

    txn->entities = NULL;
    LevelData* level = GetCurLevel();
    for (uint32_t i = 0; i < LevelDataNumEntities(level); ++i) {
        Entity* entity = arraddnptr(txn->entities, 1);
        *entity = *LevelDataGetEntity(level, i);
    }
}

static void TransactionCommit() { ActiveTxn = NULL; }

static void TransactionUndo() {
    if (arrlenu(Txns) != 0) {
        ActiveTxn = NULL;
        Transaction txn = arrpop(Txns);

        LevelData* level = GetCurLevel();
        for (uint32_t i = 0; i < LevelDataNumEntities(level); ++i) {
            Entity* entity = LevelDataGetEntity(level, i);
            *entity = txn.entities[i];
        }
    }
    TransactionCommit();
}

static void TransactionRollback() {
    if (ActiveTxn == NULL) {
        return;
    }
    TransactionUndo();
}
