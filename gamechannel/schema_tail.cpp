)";

} // anonymous namespace

void
InternalSetupGameChannelsSchema (sqlite3* db)
{
  LOG (INFO) << "Setting up the database schema for game channels...";
  CHECK_EQ (sqlite3_exec (db, SCHEMA_SQL, nullptr, nullptr, nullptr),
            SQLITE_OK);
}

} // namespace xaya
