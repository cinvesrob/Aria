
function example
     cleanupobj = onCleanup(@cleanup)
     arnetc_init
     
     c = arnetc_connect('localhost');
     if c == 0
         return
     end
     arnetc_request(c, 'gotoGoal', 'Hallway 2')
 
     ru = arnetc_new_robot_update_handler(c);
     while (true)
       m = arnetc_robot_update_get_mode(ru);
       disp(['Mode: ' m])
       s = arnetc_robot_update_get_status(ru);
       disp(['Status: ' s])
       v = arnetc_robot_update_get_vels(ru);
       disp(['Vel: ' num2str(v)])
       p = arnetc_robot_update_get_pose(ru);
       disp(['Pos: ' num2str(p)])
       %arnetc_request(c, 'LogActions')
       pause(0.5)
     end

 
     function cleanup
       disp('Example exited, cleaning up')
       arnetc_delete_robot_update_handler(ru)
       arnetc_disconnect(c)
       arnetc_shutdown
     end

end

