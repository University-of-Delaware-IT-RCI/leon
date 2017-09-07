//
// leon_worklog.c
// leon - Directory-major scratch filesystem cleanup
//
//
// The leon_worklog functions are used to produce and playback a work
// log.
//
//
// Copyright © 2013
// Dr. Jeffrey Frey
// University of Delware, IT-NSS
//
// 
// The program name is a reference to the "cleaner" named Leon in the
// movie, "The Professional."
//
// $Id: leon_worklog.c 479 2013-09-06 18:33:43Z frey $
//

#include "leon_worklog.h"
#include "leon_rm.h"
#include "leon_log.h"
#include <sqlite3.h>

//

typedef struct _leon_worklog_t {
  sqlite3             *dbh;
  bool                inMemory;
  leon_path_ref       pathToDb;
  //
  sqlite3_stmt        *addStmt;
  sqlite3_stmt        *postAddStmt;
  sqlite3_stmt        *getStmt;
  sqlite3_stmt        *postGetStmt;
} leon_worklog_t;

//

leon_worklog_t*
__leon_worklog_alloc(void)
{
  leon_worklog_t*   newWorkLog = (leon_worklog_t*)malloc(sizeof(leon_worklog_t));
  
  if ( newWorkLog ) {
    newWorkLog->dbh = NULL;
    newWorkLog->inMemory = false;
    newWorkLog->pathToDb = NULL;
  }
  return newWorkLog;
}

//

void
__leon_worklog_sqlite3_pathStartsWith(
  sqlite3_context*    context,
  int                 argc,
  sqlite3_value*      argv[]
)
{
  if ( argc == 2 ) {
    const char*       testPath = sqlite3_value_text(argv[0]);
    const char*       againstPath = sqlite3_value_text(argv[1]);
    size_t            testPathLen = ( testPath ? strlen(testPath) : 0 );
    size_t            againstPathLen = ( againstPath ? strlen(againstPath) : 0 );
    
    if ( testPathLen >= againstPathLen ) {
      if ( strncmp(testPath, againstPath, againstPathLen) == 0 ) {
        // The testPath has againstPath as its prefix; return true only if
        // the following character is '/':
        switch ( testPath[againstPathLen] ) {
          case '/':
            sqlite3_result_int(context, 1);
            break;
          default:
            sqlite3_result_int(context, 0);
            break;
        }
      }
    } else {
      sqlite3_result_int(context, 0);
    }
  } else {
    sqlite3_result_int(context, 0);
  }
}

//

int
__leon_worklog_init(
  leon_worklog_t*   aWorkLog,
  bool              isExtant
)
{
  int               rc = SQLITE_OK;
  
  if ( isExtant ) {
    rc = sqlite3_exec(aWorkLog->dbh, "DROP TABLE worklog", NULL, NULL, NULL);
    leon_log(kLeonLogDebug2, "__leon_worklog_init: Dropped extant worklog table (rc = %d)", rc);
  }
  if ( rc == SQLITE_OK ) {
    rc = sqlite3_exec(
                aWorkLog->dbh,
                "CREATE TABLE worklog (\n"
                "  pathId         INTEGER PRIMARY KEY,\n"
                "  origPath       TEXT UNIQUE NOT NULL,\n"
                "  altPath        TEXT UNIQUE NOT NULL\n"
                ")",
                NULL,
                NULL,
                NULL
              );
    leon_log(kLeonLogDebug2, "__leon_worklog_init: Created worklog table (rc = %d)", rc);
  }
  if ( rc == SQLITE_OK ) {
    rc = sqlite3_create_function(
              aWorkLog->dbh,
              "leonPathStartsWith",
              2,
              SQLITE_UTF8,
              NULL,
              __leon_worklog_sqlite3_pathStartsWith,
              NULL,
              NULL
            );
    leon_log(kLeonLogDebug2, "__leon_worklog_init: Created leon_pathStartsWith function (rc = %d)", rc);
  }
  if ( rc == SQLITE_OK ) {
    rc = sqlite3_prepare_v2(
              aWorkLog->dbh,
              "INSERT INTO worklog (origPath, altPath) VALUES (?, ?)",
              -1,
              &aWorkLog->addStmt,
              NULL
            );
    leon_log(kLeonLogDebug2, "__leon_worklog_init: Prepared 'add path' query (rc = %d)", rc);
  }
  if ( rc == SQLITE_OK ) {
    rc = sqlite3_prepare_v2(
              aWorkLog->dbh,
              "DELETE FROM worklog WHERE leonPathStartsWith(origPath, ?) <> 0",
              -1,
              &aWorkLog->postAddStmt,
              NULL
            );
    leon_log(kLeonLogDebug2, "__leon_worklog_init: Prepared 'post-add path' query (rc = %d)", rc);
  }
  if ( rc == SQLITE_OK ) {
    rc = sqlite3_prepare_v2(
              aWorkLog->dbh,
              "SELECT pathId, origPath, altPath FROM worklog ORDER BY pathId ASC LIMIT 1",
              -1,
              &aWorkLog->getStmt,
              NULL
            );
    leon_log(kLeonLogDebug2, "__leon_worklog_init: Prepared 'get path' query (rc = %d)", rc);
  }
  if ( rc == SQLITE_OK ) {
    rc = sqlite3_prepare_v2(
              aWorkLog->dbh,
              "DELETE FROM worklog WHERE pathId = ?",
              -1,
              &aWorkLog->postGetStmt,
              NULL
            );
    leon_log(kLeonLogDebug2, "__leon_worklog_init: Prepared 'post-get path' query (rc = %d)", rc);
  }
  if ( rc == SQLITE_OK ) {
    rc = sqlite3_exec(
                aWorkLog->dbh,
                "BEGIN",
                NULL,
                NULL,
                NULL
              );
    leon_log(kLeonLogDebug2, "__leon_worklog_init: Transaction started (rc = %d)", rc);
  }
  return rc;
}

//

leon_worklog_ref
leon_worklog_create(void)
{
  leon_worklog_t*     newWorkLog = __leon_worklog_alloc();
  
  if ( newWorkLog ) {
    int               rc = sqlite3_open_v2(":memory:", &newWorkLog->dbh, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    
    (rc == 0) && (rc = __leon_worklog_init(newWorkLog, false));
    if ( rc != SQLITE_OK ) {
      free((void*)newWorkLog);
      newWorkLog = NULL;
    } else {
      newWorkLog->inMemory = true;
    }
  }
  return newWorkLog;
}

//

leon_worklog_ref
leon_worklog_createWithFile(
  leon_path_ref       aPath
)
{
  leon_worklog_t*     newWorkLog = __leon_worklog_alloc();
  
  if ( newWorkLog ) {
    int               rc;
    bool              isExtant = leon_path_isFile(aPath);
    
    if ( isExtant ) {
      rc = sqlite3_open_v2(leon_path_cString(aPath), &newWorkLog->dbh, SQLITE_OPEN_READWRITE, NULL);
    } else {
      rc = sqlite3_open_v2(leon_path_cString(aPath), &newWorkLog->dbh, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    }
    (rc == SQLITE_OK) && (rc = __leon_worklog_init(newWorkLog, isExtant));
    if ( rc != SQLITE_OK ) {
      free((void*)newWorkLog);
      newWorkLog = NULL;
    } else {
      newWorkLog->pathToDb = leon_path_copy(aPath);
    }
  }
  return newWorkLog;
}

//

void
leon_worklog_destroy(
  leon_worklog_ref    aWorkLog,
  bool                doNotDelete
)
{
  if ( doNotDelete ) {
    sqlite3_exec(
        aWorkLog->dbh,
        "COMMIT",
        NULL,
        NULL,
        NULL
      );
  } else {
    sqlite3_exec(
        aWorkLog->dbh,
        "ROLLBACK",
        NULL,
        NULL,
        NULL
      );
  }
  if ( aWorkLog->dbh ) sqlite3_close(aWorkLog->dbh);
  if ( ! aWorkLog->inMemory ) {
    if ( doNotDelete ) {
      leon_log(kLeonLogInfo, "Work log not deleted: %s", leon_path_cString(aWorkLog->pathToDb));
    } else {
      int     errCode;
      
      leon_rm(aWorkLog->pathToDb, false, &errCode);
      leon_log(kLeonLogDebug1, "Work log deleted: %s (errno = %d)", leon_path_cString(aWorkLog->pathToDb), errno);
    }
    leon_path_destroy(aWorkLog->pathToDb);
  }
  free((void*)aWorkLog);
}

//

bool
leon_worklog_addPath(
  leon_worklog_ref    aWorkLog,
  leon_path_ref       inOrigPath,
  leon_path_ref       inAltPath
)
{
  const char*         origPath = leon_path_cString(inOrigPath);
  const char*         altPath = leon_path_cString(inAltPath);
  int                 rc;
  bool                result = false;
  
  // Add the path:
  rc = sqlite3_reset(aWorkLog->addStmt);
  (rc == SQLITE_OK) && (rc = sqlite3_bind_text(aWorkLog->addStmt, 1, origPath, -1, SQLITE_STATIC));
  (rc == SQLITE_OK) && (rc = sqlite3_bind_text(aWorkLog->addStmt, 2, altPath, -1, SQLITE_STATIC));
  (rc == SQLITE_OK) && (rc = sqlite3_step(aWorkLog->addStmt));
  if ( rc == SQLITE_DONE ) {
    rc = sqlite3_clear_bindings(aWorkLog->addStmt);
    
    // Clear any descendent paths:
    rc = sqlite3_reset(aWorkLog->postAddStmt);
    (rc == SQLITE_OK) && (rc = sqlite3_bind_text(aWorkLog->postAddStmt, 1, origPath, -1, SQLITE_STATIC));
    (rc == SQLITE_OK) && (rc = sqlite3_step(aWorkLog->postAddStmt));
    if ( rc != SQLITE_DONE ) {
      leon_log(kLeonLogWarning, "Unable to remove descendent paths from work log (rc = %d): %s", rc, origPath);
    } else {
      result = true;
    }
    rc = sqlite3_clear_bindings(aWorkLog->postAddStmt);
  } else {
    rc = sqlite3_clear_bindings(aWorkLog->addStmt);
    leon_log(kLeonLogError, "Unable to add path to work log (rc = %d): (%s, %s)", rc, origPath, altPath);
  }
  return result;
}

//

bool
leon_worklog_getPath(
  leon_worklog_ref    aWorkLog,
  leon_path_ref       *outAltPath
)
{
  bool                result = false;
  int                 rc;
  
  // Get the path:
  rc = sqlite3_reset(aWorkLog->getStmt);
  (rc == SQLITE_OK) && (rc = sqlite3_step(aWorkLog->getStmt));
  switch ( rc ) {
  
    case SQLITE_DONE:
      // All done:
      break;
    
    case SQLITE_ROW: {
      sqlite3_int64   pathId = sqlite3_column_int64(aWorkLog->getStmt, 0);
      const char*     origPath = (const char*)sqlite3_column_text(aWorkLog->getStmt, 1);
      const char*     altPath = (const char*)sqlite3_column_text(aWorkLog->getStmt, 2);
      
      leon_log(kLeonLogDebug2, "leon_worklog_getPath:  %s (id = %lld, orig = %s)", altPath, (long long int)pathId, origPath);
      if ( *outAltPath ) {
        leon_path_resetBasePath(*outAltPath, altPath);
      } else {
        *outAltPath = leon_path_createWithCString(altPath);
      }
      
      // Drop this row from the database:
      rc = sqlite3_reset(aWorkLog->postGetStmt);
      (rc == SQLITE_OK) && (rc = sqlite3_bind_int64(aWorkLog->postGetStmt, 1, pathId));
      (rc == SQLITE_OK) && (rc = sqlite3_step(aWorkLog->postGetStmt));
      if ( rc != SQLITE_DONE ) {
        leon_log(kLeonLogWarning, "Unable to remove path from work log (rc = %d): %lld", rc, (long long int)pathId);
      } else {
        result = true;
      }
      rc = sqlite3_clear_bindings(aWorkLog->postGetStmt);
      break;
    }
    
    default:
      leon_log(kLeonLogError, "Unable to retrieve next path from work log (rc = %d)", rc);
      break;
  }
  return result;
}

//

bool
leon_worklog_scanComplete(
  leon_worklog_ref  aWorkLog,
  bool              discardChanges
)
{
  int               rc;
  
  rc = sqlite3_exec(aWorkLog->dbh, ( discardChanges ? "ROLLBACK" : "COMMIT" ), NULL, NULL, NULL);
  (rc == SQLITE_OK) && (rc = sqlite3_exec(aWorkLog->dbh, "BEGIN", NULL, NULL, NULL));
  return (rc == SQLITE_OK) ? true : false;
}
