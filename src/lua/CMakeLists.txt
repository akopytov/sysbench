install(
  FILES bulk_insert.lua
        empty-test.lua
        oltp_common.lua
        oltp_delete.lua
        oltp_insert.lua
        oltp_point_select.lua
        oltp_read_only.lua
        oltp_read_write.lua
        oltp_update_index.lua
        oltp_update_non_index.lua
        oltp_write_only.lua
        prime-test.lua
        select_random_points.lua
        select_random_ranges.lua
  DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ)
