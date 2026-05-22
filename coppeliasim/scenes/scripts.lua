-- CoppeliaSim simROS2 placeholder script
function sysCall_init()
    local robotId = sim.getStringParam(sim.stringparam_scene_name) or "robot_0"
    
    -- Subscribers
    sub = simROS2.createSubscription('/' .. robotId .. '/cmd_vel', 'geometry_msgs/msg/Twist', 'cmdVelCallback')
    
    -- Publishers
    pubOdom = simROS2.createPublisher('/' .. robotId .. '/odom', 'nav_msgs/msg/Odometry')
    pubScan = simROS2.createPublisher('/' .. robotId .. '/scan', 'sensor_msgs/msg/LaserScan')
end

function cmdVelCallback(msg)
    -- Handle velocity command
    local lin = msg.linear.x
    local ang = msg.angular.z
    -- apply to joints
end

function sysCall_sensing()
    -- Publish odometry and scan
    -- simROS2.publish(pubOdom, odomMsg)
    -- simROS2.publish(pubScan, scanMsg)
end
