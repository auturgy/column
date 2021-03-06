/** 
* Code to get rotationaly iode to get rotationaly invariant april tag information
*/  
#include <ros/ros.h>
#include <vector>
#include <deque>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/TwistStamped.h>
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/State.h>
#include <tf/transform_datatypes.h>
#include <column/BodyPoseFilter.h>

ros::Subscriber rectified_pose_sub;
ros::Publisher pose_estimate_pub;
std::deque<geometry_msgs::PoseStamped> reading_vec; 
BodyPoseFilter bpf = BodyPoseFilter();

geometry_msgs::PoseStamped tag_position_in_camera_frame;
geometry_msgs::PoseStamped camera_position_in_tag_frame_stamped;
geometry_msgs::PoseWithCovarianceStamped filtered_pose_with_cov;
geometry_msgs::PoseStamped filtered_pose;


void tag_cb(const geometry_msgs::PoseStamped::ConstPtr& pose)
{
    int num_filtered;// = 10;
    ros::param::get("/pose_filter_min_points", num_filtered);
    double th;// = 0.3;
    ros::param::get("/pose_filter_covariance_threshold", th);
    // Get the pose from the rectified_pose topic
    camera_position_in_tag_frame_stamped = *pose;
    reading_vec.push_back(camera_position_in_tag_frame_stamped);
    if(reading_vec.size() > num_filtered)
        reading_vec.pop_front();
    // Get the pose estimate from the Body Pose Filter
    //ROS_INFO("Time Difference: %d", abs(reading_vec[0].header.stamp.sec-reading_vec[reading_vec.size()-1].header.stamp.sec));
    //ROS_INFO("First time: %d", reading_vec[0].header.stamp.sec);
    //ROS_INFO("Last time: %d", reading_vec[reading_vec.size()-1].header.stamp.sec);
    if(reading_vec.size() >= num_filtered && abs(reading_vec[0].header.stamp.sec-reading_vec[reading_vec.size()-1].header.stamp.sec)<=2)    
    {
        filtered_pose_with_cov = bpf.ransac_point(reading_vec);
        if(filtered_pose_with_cov.pose.covariance[0] <= th)
            if(filtered_pose_with_cov.pose.pose.position.x != 0.0 && filtered_pose_with_cov.pose.pose.position.y != 0.0 && filtered_pose_with_cov.pose.pose.position.z != 0)
                //ROS_INFO("Yaw after World Model: %f", filtered_pose_with_cov.pose.pose.orientation.z); 
                pose_estimate_pub.publish(filtered_pose_with_cov);
    }   
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "world_modeling");
  ros::NodeHandle nh;

  ros::Rate loop_rate(10);

  rectified_pose_sub = nh.subscribe<geometry_msgs::PoseStamped>("rectified_pose", 10, tag_cb); // Publisher of rectified pose

  pose_estimate_pub = nh.advertise<geometry_msgs::PoseWithCovarianceStamped>("filtered_pose", 10); // Publisher of rectified pose
  
  //ros::Subscriber tag_sub = nh.subscribe<geometry_msgs::Pose>("april_pose", 10, tag_cb);   //changed to april_pose_drop,updates every 1Hz
  
  ros::spin();
//  while (ros::ok()){
//    ros::spinOnce();
//  }

  return 0;
}
