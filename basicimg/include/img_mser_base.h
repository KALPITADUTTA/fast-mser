/**
* Define the result type and the base class of all  MSER algorithms.
*/

#pragma once

#include "img_connected_component.h"

namespace basicimg {

	/**
	* Define the MSER data-structure in the result of MSER detection.
	*/
	class img_mser {
	public:

		enum Memory_Type {
			Memory_Type_Moments,	// For Idiap MSER (ID-MSER) and VLFeat MSER (VF-MSER), they only extract moment information of an MSER
			Memory_Type_Self,	// Used in OpenCV MSER (CV-MSER)
			Memory_Type_Share,	// Used in CV-MSER+
			Memory_Type_Recursive,	// Used in Fast MSER V2
			Memory_Type_Share_Parallel_4, // Used in Fast MSER V1
			Memory_Type_Recursive_Parallel_4, // Used in Fast MSER V2
			Memory_Type_Range,	// Used in Fast MSER V1
		};

		img_mser() {
			m_memory_type = Memory_Type_Self;
			m_points = NULL;
			m_size = 0;
			m_gray_level = 0;
		}

		img_mser(const img_mser& other);

		~img_mser();

		void operator = (const img_mser& other);

		u8 m_gray_level;
		u8 m_memory_type;
		i32 m_size;
		mt_point* m_points;
		mt_rect m_rect;
	};

	/**
	* Define the result type of MSER detection.
	*/
	class img_multi_msers {
	public:
		img_multi_msers() {
			m_memory[0] = NULL;
			m_memory[1] = NULL;
			m_memory_size[0] = 0;
			m_memory_size[1] = 0;
		}

		img_multi_msers(const img_multi_msers& other);

		~img_multi_msers() {
			basicsys_free(m_memory[0]);
			basicsys_free(m_memory[1]);
		}

		void operator = (const img_multi_msers& other);

		vector<img_mser> m_msers[2];	// Correspond to two process passes, dark to light and light to dark
		mt_point* m_memory[2];		// Memory
		i32 m_memory_size[2];		// Memory size
	};

	/**
	* Define the base class of all MSER algorithms.
	*
	* Note that an MSER algoritm can degenerate to an ER algorithm by the following settings: 
	* m_delta = 0, ER
	* m_delta > 0, m_stable_variation = FLT_MAX, nms_similarity = -1, ER
	* m_delta > 0, m_stable_variation < FLT_MAX, nms_similarity = -1, SER
	* m_delta > 0, m_stable_variation < FLT_MAX, nms_similarity != -1, MSER
	*/
	class img_mser_base {
	public:

		img_mser_base() {
			m_delta = 1;	//if delta equals 0, the variance will not be calculated
			m_duplicated_variation = 0.1f;
			m_stable_variation = 0.5f;

			m_nms_similarity = 0;
			m_connected_type = img_Connected_Type_4_Neibour;
			m_min_point = 20;
			m_max_point_ratio = 0.25f;

			m_from_min_max[0] = sys_true;
			m_from_min_max[1] = sys_true;

			m_recursive_point_threshold = 20;
			m_parallel_thread_number = 4;

			m_channel_total_running_memory = 0;
			m_channel_total_pixel_number = 0;
		}

		virtual ~img_mser_base() {}

		void set_stable_delta(i32 delta) {
			m_delta = delta;
		}

		i32 stable_delta() {
			return m_delta;
		}

		void set_duplicated_variation(f32 variation) {
			m_duplicated_variation = variation;
		}

		void set_stable_max_variation(f32 variation) {
			m_stable_variation = variation;
		}

		void set_nms_similarity(f32 similarity) {
			m_nms_similarity = similarity;
		}

		void set_connected_type(img_Connected_Type connected_type) {
			m_connected_type = connected_type;
		}

		void set_min_max_point(i32 min_point, f32 max_point_ratio) {
			m_min_point = min_point;
			m_max_point_ratio = max_point_ratio;
		}

		void enable_extraction(b8 from_min, b8 from_max) {
			m_from_min_max[0] = from_min;
			m_from_min_max[1] = from_max;
		}

		void set_recursive_point_threshold(i32 val) {
			m_recursive_point_threshold = val;
		}

		i32 get_recursive_point_threshold() {
			return m_recursive_point_threshold;
		}

		void set_parallel_thread_number(i32 thread_number) {
			m_parallel_thread_number = thread_number;
		}

		i32 get_parallel_thread_number() {
			return m_parallel_thread_number;
		}

		/**
		* Extract MSERs from an image.
		*
		* @param res Rsults of MSER detections.
		* @param gray_src Source image. It should be a gray-scale image.
		* @param mask A mask image defining which pixels should be ignored during the detecting process.
		*/
		void extract(img_multi_msers& res, const mt_mat& gray_src, const img_mask_info<u8>& mask = img_mask_info<u8>());
		
		/**
		* Clear memory cache. When an MSER algorithm detects MSERs in an image, it needs to allocate memory for some data-structures.
		* If you want to use an MSER algorithm instance to process several images, you may not call this method.
		*/
		virtual void clear_memory_cache() {}

	protected:

		/**
		* Allocate memory in the algorithm initialization stage.
		*/
		virtual void allocate_memory(const mt_mat& src, const img_mask_info<u8>& mask) = 0;

		/**
		* Build component trees in the component tree construction stage.
		*/
		virtual void build_tree(const mt_mat& src, const img_mask_info<u8>& mask, u8 gray_mask) = 0;

		/**
		* Recognize MSERs in the MSER recognition stage.
		*/
		virtual void recognize_mser() = 0;
		
		/**
		* Extract pixels in MSERs in the pixel extraction stage.
		*/
		virtual void extract_pixel(img_multi_msers& msers, u8 gray_mask) = 0;
		

		i32 m_delta;	// When delta equals 0, the variation for an ER is -1 (not calculated) (in opencv3, it is a special case)
		f32 m_duplicated_variation;	// When m_duplicated_variation is smaller than 0, the duplicated MSERs will be retained
		f32 m_stable_variation;	// When m_stable_variation equals FLT_MAX, all ERs are MSERs.

		f32 m_nms_similarity;	// When m_nms_similarity equals -1, local minimal suppression will not be applied
		img_Connected_Type m_connected_type;	// Can be either 4-connected or 8-connected
		i32 m_min_point;	
		f32 m_max_point_ratio;
		i32 m_max_point;

		b8 m_from_min_max[2];

		i32 m_recursive_point_threshold;	// Used in Fast MSER V2
		u32 m_channel_total_pixel_number;	// Statistic information
		u64 m_channel_total_running_memory;	// Statistic information

		i32 m_parallel_thread_number;	// only valid for parallel MSER algorithms
	};

	/**
	* A factory class for creating designated MSER algorithms.
	*/
	class img_mser_alg_factory {
	public:

		img_mser_alg_factory(const string& class_name) {
			m_class_name = class_name;
		}

		img_mser_base* new_instance();

	protected:

		string m_class_name;
	};
}
