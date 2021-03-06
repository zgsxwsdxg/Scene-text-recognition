#include "utils.h"

// Runtime Functions
bool load_challenge2_test_file(Mat &src, int n)
{
	char filename[50];
	sprintf(filename, "res/ICDAR2015_test/img_%d.jpg", n);
	src = imread(filename, CV_LOAD_IMAGE_UNCHANGED);

	if (src.empty())
	{
		std::cout << n << "\tFail to open" << filename << endl;
		return false;
	}

	else if (src.cols > MAX_WIDTH || src.rows > MAX_HEIGHT)
	{
		std::cout << n << "\t" << src.rows << "," << src.cols << "\tResize the image" << endl;
		double resize_factor = (src.rows > MAX_HEIGHT) ? (double)MAX_HEIGHT / src.rows : (double)MAX_WIDTH / src.cols;

		resize(src, src, Size(src.cols*resize_factor, src.rows*resize_factor));
		return true;
	}

	//std::cout << n << "\t" << src.rows << "," << src.cols << endl;
	return true;
}

bool load_challenge2_training_file(Mat &src, int n)
{
	char filename[50];
	sprintf(filename, "res/ICDAR2015_training/%d.jpg", n);
	src = imread(filename, CV_LOAD_IMAGE_UNCHANGED);

	if (src.empty())
	{
		std::cout << n << "\tFail to open" << filename << endl;
		return false;
	}

	else if (src.cols > MAX_WIDTH || src.rows > MAX_HEIGHT)
	{
		std::cout << n << "\t" << src.rows << "," << src.cols << "\tResize the image" << endl;
		double resize_factor = (src.rows > MAX_HEIGHT) ? (double)MAX_HEIGHT / src.rows : (double)MAX_WIDTH / src.cols;

		resize(src, src, Size(src.cols*resize_factor, src.rows*resize_factor));
		return true;
	}

	std::cout << n << "\t" << src.rows << "," << src.cols << endl;
	return true;
}

void load_video_thread(VideoCapture &cap, Mat frame, Mat result, static vector<Text> *text, int *key)
{
	for (;;)
	{
		cap >> frame;
		show_result(frame, result, *text);
		*key = waitKey(1);
		if (*key >= 0) break;
	}

	return;
}

void show_result(Mat& src, Mat& result_img, vector<Text> &text, vector<double> &times, ERs &tracked, vector<ERs> &strong, vector<ERs> &weak, vector<ERs> &all, vector<ERs> &pool)
{
	Mat all_img = src.clone();
	Mat pool_img = src.clone();
	Mat strong_img = src.clone();
	Mat weak_img = src.clone();
	Mat tracked_img = src.clone();
	result_img = src.clone();
	for (int i = 0; i < pool.size(); i++)
	{
		for (auto it : all[i])
			rectangle(all_img, it->bound, Scalar(255, 0, 0));

		for (auto it : pool[i])
			rectangle(pool_img, it->bound, Scalar(255, 0, 0));

		for (auto it : weak[i])
			rectangle(weak_img, it->bound, Scalar(0, 0, 255));

		for (auto it : strong[i])
			rectangle(strong_img, it->bound, Scalar(0, 255, 0));
	}

	for (auto it : tracked)
	{
		rectangle(tracked_img, it->bound, Scalar(255, 0, 255));
	}

	for (auto it : text)
	{
		/*if (abs(it.slope) > 0.05)
		{
			vector<Point> points;
			for (auto it_er : it.ers)
			{
				points.push_back(Point(it_er->bound.x, it_er->bound.y));
				points.push_back(Point(it_er->bound.br().x, it_er->bound.br().y));
				points.push_back(Point(it_er->bound.x, it_er->bound.br().y));
				points.push_back(Point(it_er->bound.br().x, it_er->bound.y));
			}
			
			RotatedRect ro_rect = minAreaRect(points);
			Point2f vertices[4];
			ro_rect.points(vertices);
			for (int i = 0; i < 4; i++)
				line(result_img, vertices[i], vertices[(i + 1) % 4], Scalar(0, 255, 255), 2);
		}

		else
		{
			rectangle(result_img, it.box, Scalar(0, 255, 255), 2);
		}*/
		rectangle(result_img, it.box, Scalar(0, 255, 255), 2);
	}

	for (auto it : text)
	{
#ifndef DO_OCR
		//rectangle(result_img, Rect(it.box.tl().x, it.box.tl().y-20, 53, 19), Scalar(30, 30, 200), CV_FILLED);
		//putText(result_img, "Text", Point(it.box.tl().x, it.box.tl().y-4), FONT_HERSHEY_COMPLEX_SMALL, 1, Scalar(255, 255, 255), 1);
		circle(result_img, Point(it.box.tl().x + 5, it.box.tl().y - 12), 10, Scalar(30, 30, 200), CV_FILLED);
		putText(result_img, "T", Point(it.box.tl().x - 2, it.box.tl().y - 5), FONT_HERSHEY_COMPLEX_SMALL, 1, Scalar(255, 255, 255), 1);
#else
		Size text_size = getTextSize(it.word, FONT_HERSHEY_COMPLEX_SMALL, 1, 1, 0);
		rectangle(result_img, Rect(it.box.tl().x, it.box.tl().y - 20, text_size.width, text_size.height + 5), Scalar(30, 30, 200, 0), CV_FILLED);
		putText(result_img, it.word, Point(it.box.tl().x, it.box.tl().y - 4), FONT_HERSHEY_COMPLEX_SMALL, 1, Scalar(0xff, 0xff, 0xff), 1);
#endif
	}


	if (!times.empty())
	{
	#if defined(WEBCAM_MODE) || defined(VIDEO_MODE)
		draw_FPS(result_img, times.back());
	#endif

		cout << "ER extraction = " << times[0] * 1000 << "ms\n"
			<< "Non-maximum suppression = " << times[1] * 1000 << "ms\n"
			<< "Classification = " << times[2] * 1000 << "ms\n"
			<< "Character tracking = " << times[3] * 1000 << "ms\n"
			<< "Character grouping = " << times[4] * 1000 << "ms\n"
	#ifdef DO_OCR
			<< "OCR = " << times[5] * 1000 << "ms\n"
	#endif
			<< "Total execution time = " << times[6] * 1000 << "ms\n\n";
	}

	if (!all.empty())
	{
		double alpha = 0.7;
		addWeighted(all_img, alpha, src, 1.0 - alpha, 0.0, all_img);
		cv::imshow("all", all_img);
	}
	if (!pool.empty())
		cv::imshow("pool", pool_img);
	if (!weak.empty())
		cv::imshow("weak", weak_img);
	if (!strong.empty())
		cv::imshow("strong", strong_img);
	if (!tracked.empty())
		cv::imshow("tracked", tracked_img);
	cv::imshow("result", result_img);
	waitKey(1);

#ifdef IMAGE_MODE
	if (!all.empty())
		imwrite("all.png", all_img);
	if (!pool.empty())
		imwrite("pool.png", pool_img);
	if (!weak.empty())
		imwrite("weak.png", weak_img);
	if (!strong.empty())
		imwrite("strong.png", strong_img);
	if (!tracked.empty())
		imwrite("tracked.png", tracked_img);
	imwrite("result.png", result_img);
	waitKey(0);
#endif
	
}

void draw_FPS(Mat& src, double time)
{
	static int counter = 0;
	static double avg_time = 0;
	static char fps_text[20];

	if (counter > 10)
	{
		double fps = 1 / (avg_time / 10.0);
		sprintf(fps_text, "FPS: %.1f", fps);
		counter = 0;
		avg_time = 0;
	}
	avg_time += time;
	++counter;

	putText(src, fps_text, Point(10, 25), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 0), 2);
}

// Testing Functions
void draw_linear_time_MSER(string img_name)
{
	Mat input = imread(img_name);

	int pixel_count = 0;
	VideoWriter writer;
	//writer.open("Linear time MSER.avi", CV_FOURCC('M', 'J', 'P', 'G'), 30, input.size());
	writer.open("Linear time MSER.wmv", CV_FOURCC('W', 'M', 'V', '2'), 30, input.size());


	Mat color = Mat::zeros(input.rows, input.cols, CV_8UC3);
	Mat gray;
	cvtColor(input, gray, COLOR_BGR2GRAY);
	const int width = gray.cols;
	const int height = gray.rows;
	const int highest_level = 255 + 1;
	const uchar *imgData = gray.data;

	//!< 1. Clear the accessible pixel mask, the heap of boundary pixels and the component
	bool *pixel_accessible = new bool[height*width]();
	vector<int> boundary_pixel[256];
	vector<int> boundary_edge[256];
	vector<ER *>er_stack;

	int priority = highest_level;


	//!< 1-2. push a dummy-component onto the stack, 
	//!<	  with grey-level heigher than any allowed in the image
	er_stack.push_back(new ER(256, 0, 0, 0));


	//!< 2. make the top-right corner the source pixel, get its gray level and mark it accessible
	int current_pixel = 0;
	int current_edge = 0;
	int current_level = imgData[current_pixel];
	pixel_accessible[current_pixel] = true;


step_3:
	int x = current_pixel % width;
	int y = current_pixel / width;

	//!< 3. push an empty component with current_level onto the component stack
	er_stack.push_back(new ER(current_level, current_pixel, x, y));


	for (;;)
	{
		//!< 4. Explore the remaining edges to the neighbors of the current pixel, in order, as follows : 
		//!<	For each neighbor, check if the neighbor is already accessible.If it
		//!<	is not, mark it as accessible and retrieve its grey - level.If the grey - level is not
		//!<	lower than the current one, push it onto the heap of boundary pixels.If on
		//!<	the other hand the grey - level is lower than the current one, enter the current
		//!<	pixel back into the queue of boundary pixels for later processing(with the
		//!<	next edge number), consider the new pixel and its grey - level and go to 3.
		int neighbor_pixel;
		int neighbor_level;


		for (; current_edge < 4; current_edge++)
		{
			switch (current_edge)
			{
			case 0: neighbor_pixel = (x + 1 < width) ? current_pixel + 1 : current_pixel;	break;
			case 1: neighbor_pixel = (y + 1 < height) ? current_pixel + width : current_pixel;	break;
			case 2: neighbor_pixel = (x > 0) ? current_pixel - 1 : current_pixel;	break;
			case 3: neighbor_pixel = (y > 0) ? current_pixel - width : current_pixel;	break;
			default: break;
			}

			if (!pixel_accessible[neighbor_pixel] && neighbor_pixel != current_pixel)
			{
				pixel_accessible[neighbor_pixel] = true;
				neighbor_level = imgData[neighbor_pixel];

				if (neighbor_level >= current_level)
				{
					boundary_pixel[neighbor_level].push_back(neighbor_pixel);
					boundary_edge[neighbor_level].push_back(0);

					if (neighbor_level < priority)
						priority = neighbor_level;

					int nx = neighbor_pixel % width;
					int ny = neighbor_pixel / width;
					color.at<uchar>(ny, nx * 3) = 0;
					color.at<uchar>(ny, nx * 3 + 1) = 255;
					color.at<uchar>(ny, nx * 3 + 2) = 0;
				}
				else
				{
					boundary_pixel[current_level].push_back(current_pixel);
					boundary_edge[current_level].push_back(current_edge + 1);

					if (current_level < priority)
						priority = current_level;

					color.at<uchar>(y, x * 3) = 0;
					color.at<uchar>(y, x * 3 + 1) = 255;
					color.at<uchar>(y, x * 3 + 2) = 0;

					current_pixel = neighbor_pixel;
					current_level = neighbor_level;
					current_edge = 0;
					goto step_3;
				}
			}
		}




		//!< 5. Accumulate the current pixel to the component at the top of the stack 
		//!<	(water saturates the current pixel).
		er_stack.back()->area++;
		int x1 = min(er_stack.back()->bound.x, x);
		int x2 = max(er_stack.back()->bound.br().x - 1, x);
		int y1 = min(er_stack.back()->bound.y, y);
		int y2 = max(er_stack.back()->bound.br().y - 1, y);
		er_stack.back()->bound.x = x1;
		er_stack.back()->bound.y = y1;
		er_stack.back()->bound.width = x2 - x1 + 1;
		er_stack.back()->bound.height = y2 - y1 + 1;

		color.at<uchar>(y, x * 3) = input.at<uchar>(y, x * 3);
		color.at<uchar>(y, x * 3 + 1) = input.at<uchar>(y, x * 3 + 1);
		color.at<uchar>(y, x * 3 + 2) = input.at<uchar>(y, x * 3 + 2);
		/*color.at<uchar>(y, x * 3) = current_level;
		color.at<uchar>(y, x * 3 + 1) = current_level;
		color.at<uchar>(y, x * 3 + 2) = current_level;*/
		pixel_count++;
		if (pixel_count % 300 == 0)
		{
			imshow("Linear time MSER", color);
			writer << color;
			waitKey(1);
		}


		//!< 6. Pop the heap of boundary pixels. If the heap is empty, we are done. If the
		//!<	returned pixel is at the same grey - level as the previous, go to 4	
		if (priority == highest_level)
		{
			delete[] pixel_accessible;
			writer.release();
			waitKey(0);
			return;
		}


		int new_pixel = boundary_pixel[priority].back();
		int new_edge = boundary_edge[priority].back();
		int new_pixel_grey_level = imgData[new_pixel];

		boundary_pixel[priority].pop_back();
		boundary_edge[priority].pop_back();

		while (boundary_pixel[priority].empty() && priority < highest_level)
			priority++;

		current_pixel = new_pixel;
		current_edge = new_edge;
		x = current_pixel % width;
		y = current_pixel / width;

		if (new_pixel_grey_level != current_level)
		{
			
			current_level = new_pixel_grey_level;

			do
			{
				ER *top = er_stack.back();
				ER *second_top = er_stack.end()[-2];
				er_stack.pop_back();

				
				if (new_pixel_grey_level < second_top->level)
				{
					er_stack.push_back(new ER(new_pixel_grey_level, top->pixel, top->x, top->y));
					er_stack.back()->area += top->area;

					const int x1 = min(er_stack.back()->bound.x, top->bound.x);
					const int x2 = max(er_stack.back()->bound.br().x - 1, top->bound.br().x - 1);
					const int y1 = min(er_stack.back()->bound.y, top->bound.y);
					const int y2 = max(er_stack.back()->bound.br().y - 1, top->bound.br().y - 1);

					er_stack.back()->bound.x = x1;
					er_stack.back()->bound.y = y1;
					er_stack.back()->bound.width = x2 - x1 + 1;
					er_stack.back()->bound.height = y2 - y1 + 1;

					
					top->next = er_stack.back()->child;
					er_stack.back()->child = top;
					top->parent = er_stack.back();

					break;
				}

				second_top->area += top->area;

				const int x1 = min(second_top->bound.x, top->bound.x);
				const int x2 = max(second_top->bound.br().x - 1, top->bound.br().x - 1);
				const int y1 = min(second_top->bound.y, top->bound.y);
				const int y2 = max(second_top->bound.br().y - 1, top->bound.br().y - 1);

				second_top->bound.x = x1;
				second_top->bound.y = y1;
				second_top->bound.width = x2 - x1 + 1;
				second_top->bound.height = y2 - y1 + 1;


				top->next = second_top->child;
				second_top->child = top;
				top->parent = second_top;

			}
			while (new_pixel_grey_level > er_stack.back()->level);
		}
	}
}


void draw_multiple_channel(string img_name)
{
	ERFilter *erFilter = new ERFilter(1, MIN_ER_AREA, MAX_ER_AREA, NMS_STABILITY_T, NMS_OVERLAP_COEF);
	Mat input = imread(img_name);
	


	Mat Ycrcb;
	vector<Mat> channel;
	erFilter->compute_channels(input, Ycrcb, channel);

	ERs root;
	vector<ERs> all;
	vector<ERs> pool;
	root.resize(channel.size());
	all.resize(channel.size());
	pool.resize(channel.size());

	for (int i = 0; i < channel.size(); i++)
	{
		root[i] = erFilter->er_tree_extract(channel[i]);
		erFilter->non_maximum_supression(root[i], all[i], pool[i], channel[i]);

		for (int j = 0; j < pool[i].size(); j++)
		{
			rectangle(channel[i%3], pool[i][j]->bound, Scalar(0));
		}
	}


	imshow("Y", channel[0]);
	imshow("Cr", channel[1]);
	imshow("Cb", channel[2]);
	imwrite("Y.jpg", channel[0]);
	imwrite("Cr.jpg", channel[1]);
	imwrite("Cb.jpg", channel[2]);
	waitKey(0);
}


void output_MSER_time(string img_name)
{
	fstream fout = fstream("time.txt", fstream::out);
	ERFilter *erFilter = new ERFilter(1, 100, 1.0E7, NMS_STABILITY_T, NMS_OVERLAP_COEF);
	Mat input = imread(img_name, IMREAD_GRAYSCALE);
	double coef = 0.181;
	Mat tmp;
	while (tmp.total() <= 1.0E7)
	{
		resize(input, tmp, Size(), coef, coef);

		chrono::high_resolution_clock::time_point start, end;
		start = chrono::high_resolution_clock::now();

		ERs root(5);
		for (int i = 0; i < root.size(); i++)
		{
			root[i] = erFilter->er_tree_extract(tmp);
		}
			

		end = chrono::high_resolution_clock::now();

		for (int i = 0; i < root.size(); i++)
			erFilter->er_delete(root[i]);

		std::cout << "pixel number: " << tmp.total();
		std::cout << "\ttime: " << chrono::duration<double>(end - start).count() * 1000 / root.size() << "ms\n";
		fout << tmp.total() << " " << chrono::duration<double>(end - start).count() * 1000 / root.size() << endl;
		coef += 0.01;
	}
	erFilter->er_tree_extract(input);
}



void output_classifier_ROC(string classifier_name, string test_file)
{

}


void output_optimal_path(string img_name)
{
	Mat src = imread("img_1.jpg");
	Mat Ycrcb;
	vector<Mat> channel;

	ERs root, tracked;
	vector<ERs> all, pool, strong, weak;
	vector<Text> text;
	ERFilter er_filter;
	
	er_filter.compute_channels(src, Ycrcb, channel);

	root.resize(channel.size());
	all.resize(channel.size());
	pool.resize(channel.size());
	strong.resize(channel.size());
	weak.resize(channel.size());

#pragma omp parallel for
	for (int i = 0; i < channel.size(); i++)
	{
		root[i] = er_filter.er_tree_extract(channel[i]);
		er_filter.non_maximum_supression(root[i], all[i], pool[i], channel[i]);
		er_filter.classify(pool[i], strong[i], weak[i], channel[i]);
	}

	er_filter.er_track(strong, weak, tracked, channel, Ycrcb);
	er_filter.er_ocr(tracked, channel, text);
}


vector<Vec4i> load_gt(int n)
{
	char filename[50];
	sprintf(filename, "res/ICDAR2015_test_GT/gt_img_%d.txt", n);
	fstream fin(filename, fstream::in);
	if (!fin.is_open())
	{
		std::cout << "Error: Ground Truth file " << n << " is not opened!!" << endl;
		return vector<Vec4i>();
	}

	char picname[50];
	sprintf(picname, "res/ICDAR2015_test/img_%d.jpg", n);
	Mat src = imread(picname, CV_LOAD_IMAGE_UNCHANGED);


	vector<string> data;
	while (!fin.eof())
	{
		string buf;
		fin >> buf;
		data.push_back(buf);
	}

	// the last data would be eof, erase it
	data.pop_back();
	for (int i = 0; i < data.size(); i++)
	{
		data[i].pop_back();
		if (i % 5 == 4)
			data[i].erase(data[i].begin());
	}

	double resize_factor = 1.0;
	if (src.cols > MAX_WIDTH || src.rows > MAX_HEIGHT)
	{
		resize_factor = (src.rows > MAX_HEIGHT) ? (double)MAX_HEIGHT / src.rows : (double)MAX_WIDTH / src.cols;
	}

	// convert string numbers to bounding box, format as shown below
	// 0 0 100 100 HAHA
	// first 2 numbers represent the coordinate of top left point
	// last 2 numbers represent the coordinate of bottom right point
	vector<Rect> bbox;
	for (int i = 0; i < data.size(); i += 5)
	{
		int x1 = stoi(data[i]);
		int y1 = stoi(data[i + 1]);
		int x2 = stoi(data[i + 2]);
		int y2 = stoi(data[i + 3]);

		x1 *= resize_factor;
		y1 *= resize_factor;
		x2 *= resize_factor;
		y2 *= resize_factor;

		bbox.push_back(Rect(Point(x1, y1), Point(x2, y2)));
	}

	// merge the bounding box that could in the same group
	/*for (int i = 0; i < bbox.size(); i++)
	{
		for (int j = i+1; j < bbox.size(); j++)
		{
		if (abs(bbox[i].y - bbox[j].y) < bbox[i].height &&
			abs(bbox[i].y - bbox[j].y) < 0.2 * src.cols * resize_factor &&
			(double)min(bbox[i].height, bbox[j].height) / (double)max(bbox[i].height, bbox[j].height) > 0.7)
			{
				int x1 = min(bbox[i].x, bbox[j].x);
				int y1 = min(bbox[i].y, bbox[j].y);
				int x2 = max(bbox[i].br().x, bbox[j].br().x);
				int y2 = max(bbox[i].br().y, bbox[j].br().y);
				bbox[i] = Rect(Point(x1, y1), Point(x2, y2));
				bbox.erase(bbox.begin() + j);
				j--;
			}
		}
	}*/

	vector<Vec4i> gt;
	for (int i = 0; i < bbox.size(); i++)
	{
		gt.push_back(Vec4i(bbox[i].x, bbox[i].y, bbox[i].width, bbox[i].height));
	}

	fin.close();
	return gt;
}


Vec6d calc_detection_rate(int n, vector<Text> &text)
{
	vector<Vec4i> gt = load_gt(n);
	vector<Rect> gt_box;
	for (int i = 0; i < gt.size(); i++)
	{
		gt_box.push_back(Rect(gt[i][0], gt[i][1], gt[i][2], gt[i][3]));
	}


	vector<bool> detected(gt_box.size());
	double tp = 0;
	for (int i = 0; i < gt_box.size(); i++)
	{
		for (int j = 0; j < text.size(); j++)
		{
			Rect overlap = gt_box[i] & text[j].box;
			Rect union_box = gt_box[i] | text[j].box;

			if ((double)overlap.area() / union_box.area() > 0.3)
			{
				detected[i] = true;
				tp++;
				break;
			}
		}
	}

	double recall = tp / detected.size();
	double precision = (!text.empty()) ? tp / text.size() : 0;
	double harmonic_mean = (precision != 0) ? (recall*precision) * 2 / (recall + precision) : 0;

	return Vec6d(tp, detected.size(), text.size(), recall, precision, harmonic_mean);
}


void calc_recall_rate()
{
	ERFilter* er_filter = new ERFilter(2, 50, MAX_ER_AREA, NMS_STABILITY_T, NMS_OVERLAP_COEF, MIN_OCR_PROBABILITY);
	er_filter->stc = new CascadeBoost("er_classifier/strong.classifier");
	er_filter->wtc = new CascadeBoost("er_classifier/weak.classifier");
	er_filter->ocr = new OCR("ocr_classifier/OCR.model", OCR_IMG_L, OCR_FEATURE_L);
	er_filter->load_tp_table("dictionary/tp_table.txt");

	Ptr<MSER> ms = MSER::create();
	
	
	int img_count = 0;
	double overlap_coef = 0.5;
	vector<double> recall_vec(7, 0);			// all ER, nms, strong, weak, classify, track, MSER
	vector<double> precision_vec(7, 0);
	vector<int> candidate_vec(7, 0);

	for (int n = 100; n <= 328; n++)
	{
		Mat src;
		Mat result;
		if (!load_challenge2_training_file(src, n))	continue;

		Mat Ycrcb;
		vector<Mat> channel;
		er_filter->compute_channels(src, Ycrcb, channel);

		ERs root(channel.size());
		vector<ERs> all(channel.size());
		vector<ERs> pool(channel.size());
		vector<ERs> strong(channel.size());
		vector<ERs> weak(channel.size());
		ERs tracked;

		vector<vector<Point>> regions;
		vector<Rect> mser_bbox;
		ms->detectRegions(Ycrcb, regions, mser_bbox);

#pragma omp parallel for
		for (int i = 0; i < channel.size(); i++)
		{
			root[i] = er_filter->er_tree_extract(channel[i]);
			er_filter->non_maximum_supression(root[i], all[i], pool[i], channel[i]);
			er_filter->classify(pool[i], strong[i], weak[i], channel[i]);
			//ms->detectRegions(channel[i], regions, mser_bbox);
		}

		er_filter->er_track(strong, weak, tracked, channel, Ycrcb);

		vector<Rect> gt_box;
		char buf[60];
		sprintf(buf, "res/ICDAR2015_training_GT/%d_GT.txt", n);
		
		ifstream infile(buf);
		string line;
		while (getline(infile, line))
		{
			istringstream iss(line);
			
			int d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
			if (!(iss >> d0 >> d1 >> d2 >> d3 >> d4 >> d5 >> d6 >> d7 >> d8))
			{
				// encounter a blank line
				continue;
			}
			
			gt_box.push_back(Rect(Point(d5, d6), Point(d7, d8)));
		}

		// flatten each stage output
		vector<ERs> flat(7);	// all ER, nms, strong, weak, classify, track
		for (auto it : all)
			flat[0].insert(flat[0].end(), it.begin(), it.end());
		for (auto it : pool)
			flat[1].insert(flat[1].end(), it.begin(), it.end());
		for (auto it : strong)
			flat[2].insert(flat[2].end(), it.begin(), it.end());
		for (auto it : weak)
			flat[3].insert(flat[3].end(), it.begin(), it.end());
		flat[4].insert(flat[4].end(), flat[2].begin(), flat[2].end());
		flat[4].insert(flat[4].end(), flat[3].begin(), flat[3].end());
		flat[5].insert(flat[5].end(), tracked.begin(), tracked.end());
		flat[6].resize(mser_bbox.size());

		vector<vector<bool>> matches(7, vector<bool>(gt_box.size(), false));	// all, nms, strong, weak, classify, track
		vector<int> recall_count(7, 0);
		vector<int> match_count(7, 0);

		
		for (int b = 0; b < gt_box.size(); b++)
		{
			// check matches and misses for my algorithm
			for (int i = 0; i < 6; i++)
			{
				for (int j = 0; j < flat[i].size(); j++)
				{
					int overlap_area = (gt_box[b] & flat[i][j]->bound).area();
					int union_area = (gt_box[b] | flat[i][j]->bound).area();
					double overlap_over_union = (double)overlap_area / (double)union_area;
					if (overlap_over_union > overlap_coef)
					{
						matches[i][b] = true;
						match_count[i]++;
					}
				}
			}

			for (int j = 0; j < mser_bbox.size(); j++)
			{
				int overlap_area = (gt_box[b] & mser_bbox[j]).area();
				int union_area = (gt_box[b] | mser_bbox[j]).area();
				double overlap_over_union = (double)overlap_area / (double)union_area;
				if (overlap_over_union > overlap_coef)
				{
					matches[6][b] = true;
					match_count[6]++;
				}
			}
		}

		// count matches for every stage
		for (int i = 0; i < 7; i++)
		{
			for (auto it : matches[i])
			{
				if (it == true)
					++recall_count[i];
			}
		}
		
		for (int i = 0; i < flat.size(); i++)
		{
			if (flat[i].empty())
			{
				flat[i].push_back(new ER());
			}
		}

		cout << "all candidate : " << flat[0].size() <<" recall : " << recall_count[0] / (double)gt_box.size() << "    precision : " << match_count[0] / (double)flat[0].size() << endl;
		cout << "nms candidate : " << flat[1].size() << " recall : " << recall_count[1] / (double)gt_box.size() << "    precision : " << match_count[1] / (double)flat[1].size() << endl;
		cout << "strong candidate : " << flat[2].size() << " recall : " << recall_count[2] / (double)gt_box.size() << "    precision : " << match_count[2] / (double)flat[2].size() << endl;
		cout << "weak candidate : " << flat[3].size() << " recall : " << recall_count[3] / (double)gt_box.size() << "    precision : " << match_count[3] / (double)flat[3].size() << endl;
		cout << "classify candidate : " << flat[4].size() << " recall : " << recall_count[4] / (double)gt_box.size() << "    precision : " << match_count[4] / (double)flat[4].size() << endl;
		cout << "track candidate : " << flat[5].size() << " recall : " << recall_count[5] / (double)gt_box.size() << "    precision : " << match_count[5] / (double)flat[5].size() << endl;
		cout << "MSER candidate : " << flat[6].size() << " recall : " << recall_count[6] / (double)gt_box.size() << "    precision : " << match_count[6] / (double)flat[6].size() << endl << endl;

		img_count++;
		for (int i = 0; i < 7; i++)
		{
			recall_vec[i] += recall_count[i] / (double)gt_box.size();
			precision_vec[i] += match_count[i] / (double)flat[i].size();
			candidate_vec[i] += flat[i].size();
		}
	}

	std::cout << "Final candidates, recall and precision :" << endl;
	std::cout << "all candidate : " << candidate_vec[0]<< "    recall : " << recall_vec[0] / img_count << "    precision : " << precision_vec[0] / img_count << endl;
	std::cout << "nms candidate : " << candidate_vec[1] << "    recall : " << recall_vec[1] / img_count << "    precision : " << precision_vec[1] / img_count << endl;
	std::cout << "strong candidate : " << candidate_vec[2] << "    recall : " << recall_vec[2] / img_count << "    precision : " << precision_vec[2] / img_count << endl;
	std::cout << "weak candidate : " << candidate_vec[3] << "    recall : " << recall_vec[3] / img_count << "    precision : " << precision_vec[3] / img_count << endl;
	std::cout << "classify candidate : " << candidate_vec[4] << "    recall : " << recall_vec[4] / img_count << "    precision : " << precision_vec[4] / img_count << endl;
	std::cout << "track candidate : " << candidate_vec[5] << "    recall : " << recall_vec[5] / img_count << "    precision : " << precision_vec[5] / img_count << endl;
	std::cout << "MSER candidate : " << candidate_vec[6] << "    recall : " << recall_vec[6] / img_count << "    precision : " << precision_vec[6] / img_count << endl << endl;
}


void save_deteval_xml(vector<vector<Text>> &text, string det_name)
{
	//remove("gt.xml");
	//remove("det.xml");

	fstream fgt("others/deteval/gt.xml", fstream::out);
	fstream fdet(det_name, fstream::out);

	fgt << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl << "<tagset>" << endl;
	fdet << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl << "<tagset>" << endl;

	for (int i = 1; i <= 233; i++)
	{
		vector<Vec4i> gt = load_gt(i);
		if (gt.empty()) continue;

		// ground truth
		fgt << "\t<image>" << endl << "\t\t" << "<imageName>img_" << i << ".jpg</imageName>" << endl;
		fgt << "\t\t<taggedRectangles>" << endl;
		for (int j = 0; j < gt.size(); j++)
		{
			fgt << "\t\t\t<taggedRectangle x=\"" << gt[j][0] << "\" y=\"" << gt[j][1] << "\" width=\"" << gt[j][2] << "\" height=\"" << gt[j][3] << "\" offset=\"0\" />" << endl;
		}
		fgt << "\t\t</taggedRectangles>" << endl;
		fgt << "\t</image>" << endl;


		// detections
		fdet << "\t<image>" << endl << "\t\t" << "<imageName>img_" << i << ".jpg</imageName>" << endl;
		fdet << "\t\t<taggedRectangles>" << endl;
		for (int j = 0; j < text[i-1].size(); j++)
		{
			fdet << "\t\t\t<taggedRectangle x=\"" << text[i-1][j].box.x << "\" y=\"" << text[i - 1][j].box.y << "\" width=\"" << text[i - 1][j].box.width << "\" height=\"" << text[i - 1][j].box.height << "\" offset=\"0\" />" << endl;
		}
		fdet << "\t\t</taggedRectangles>" << endl;
		fdet << "\t</image>" << endl;
	}

	fgt << "</tagset>";
	fdet << "</tagset>";
}


void test_best_detval()
{
	ERFilter* er_filter = new ERFilter(THRESHOLD_STEP, MIN_ER_AREA, MAX_ER_AREA, NMS_STABILITY_T, NMS_OVERLAP_COEF, MIN_OCR_PROBABILITY);
	er_filter->stc = new CascadeBoost("er_classifier/cascade1.classifier");
	er_filter->wtc = new CascadeBoost("er_classifier/weak.classifier");
	er_filter->ocr = new OCR("ocr_classifier/OCR.model", OCR_IMG_L, OCR_FEATURE_L);
	er_filter->load_tp_table("dictionar/yttp_table.txt");


	for (int thresh_step = 9; thresh_step <= 16; thresh_step++)
	{
		for (int min_area = 20; min_area <= 200; min_area+=10)
		{
			cout << "Test " << thresh_step << " " << min_area << endl;
			er_filter->set_thresh_step(thresh_step);
			er_filter->set_min_area(min_area);
			
			vector<vector<Text>> det_text;
			for (int n = 1; n <= 233; n++)
			{
				Mat src;
				Mat result;
				if (!load_challenge2_test_file(src, n))	continue;

				ERs root;
				vector<ERs> all;
				vector<ERs> pool;
				vector<ERs> strong;
				vector<ERs> weak;
				ERs tracked;
				vector<Text> text;

				vector<double> times = er_filter->text_detect(src, root, all, pool, strong, weak, tracked, text);

				det_text.push_back(text);
			}

			string det_name = "others/deteval/det_" + to_string(thresh_step) + "_" + to_string(min_area) + ".xml";
			save_deteval_xml(det_text, det_name);
		}
	}
}


void make_video_ground_truth()
{
	fstream f_gt("video_result/result3/gt.txt", fstream::out);

	for (int i = 0; i <= 3133; i++)
	{
		f_gt << i;

		/*if (i >= 2 && i <= 139)
		{
			f_gt << ',' << "Official" << ',' << "GRE" << ',' << "VERBAL" << ',' << "REASONING" << ',' << "Practice Questions";
		}

		if (i >= 142 && i <= 510)
		{
			f_gt << ',' << "OpenCV";
		}

		if (i >= 524 && i <= 638)
		{
			f_gt << ',' << "MicroC OS II" << ',' << "The Real Time Kernel" << ',' << "Second Edition";
		}*/


		/*if (i >= 2 && i <= 793)
		{
			f_gt << ',' << "P5QL EM" << ',' << "Motherboard";
		}*/


		if (i >= 2 && i <= 489)
		{
			f_gt << ',' << "EVI" << ',' << "D70" << ',' << "series";
		}

		if (i >= 518 && i <= 1013)
		{
			f_gt << ',' << "SONY";
		}

		if (i >= 1044 && i <= 1505)
		{
			f_gt << ',' << "QUALITY" << ',' << "A4" << ',' << "70" << ',' << "500" << ',' << "5";
		}

		if (i >= 1588 && i <= 2089)
		{
			f_gt << ',' << "NTUST";
		}

		if (i >= 2116 && i <= 2665)
		{
			f_gt << ',' << "REALTEK";
		}

		if (i >= 2730 && i <= 3133)
		{
			f_gt << ',' << "BenQ";
		}

		f_gt << endl;
	}
}

void calc_video_result()
{
	fstream f_gt("video_result/result3/gt.txt", fstream::in);
	fstream f_det("video_result/result3/det.txt", fstream::in);

	string line;
	vector<vector<string>> gt;
	vector<vector<string>> det;

	while (getline(f_gt, line))
	{
		istringstream iss(line);
		string s;

		gt.push_back(vector<string>(0));
		while (getline(iss, s, ','))
		{
			gt.back().push_back(s);
		}
	}

	while (getline(f_det, line))
	{
		istringstream iss(line);
		string s;

		det.push_back(vector<string>(0));
		while (getline(iss, s, ','))
		{
			det.back().push_back(s);
		}
	}

	if (gt.size() != det.size())
	{
		cerr << "gt frame count and det frame count are different!" << endl;
		return;
	}
		

	double correct_thresh = 0.7;
	int gt_count = 0;
	int det_count = 0;
	int tp = 0;
	int fp = 0;
	int fn = 0;
	for (int i = 0; i < gt.size(); i++)
	{
		gt_count += gt[i].size() - 1;
		det_count += det[i].size() - 1;

		vector<bool> gt_is_match(gt[i].size(), false);
		vector<bool> det_is_match(det[i].size(), false);
		for (int j = 1; j < det[i].size(); j++)
		{
			for (int k = 1; k < gt[i].size(); k++)
			{
				int edit_distance = levenshtein_distance(gt[i][k], det[i][j]);
				double correct_rate = 1 - (double)edit_distance / max(det[i][j].size(), gt[i][k].size());

				if (correct_rate > correct_thresh)
				{
					gt_is_match[k] = true;
					det_is_match[j] = true;
				}
			}
		}

		for (int k = 1; k < gt_is_match.size(); k++)
		{
			if (!gt_is_match[k])
				fn++;
		}

		for (int j = 1; j < det_is_match.size(); j++)
		{
			if (det_is_match[j])
				tp++;
			else
				fp++;
		}
	}

	double recall = tp / (double)gt_count;
	double precision = tp / (double)det_count;
	double f_score = 2 * recall*precision / (recall + precision);
	cout << "Ground truth count: " << gt_count << endl
		<< "Detected count: " << det_count << endl
		<< "True postive: " << tp << endl
		<< "False postive: " << fp << endl
		<< "Miss detected: " << fn << endl
		<< "Recall: " << recall << endl
		<< "Precision: " << precision << endl
		<< "f-score: " << f_score << endl;

	fstream result_file("video_result/result3/result_file.txt", fstream::out);
	result_file << "Ground truth count: " << gt_count << endl
		<< "Detected count: " << det_count << endl
		<< "True postive: " << tp << endl
		<< "False postive: " << fp << endl
		<< "Miss detected: " << fn << endl
		<< "Recall: " << recall << endl
		<< "Precision: " << precision << endl
		<< "f-score: " << f_score << endl;
}

//==================================================
//=============== Training Function ================
//==================================================
void train_classifier()
{
	TrainingData *td1 = new TrainingData();
	TrainingData *td2 = new TrainingData();
	AdaBoost adb1(AdaBoost::REAL, AdaBoost::DECISION_STUMP, 30);
	AdaBoost adb2(AdaBoost::REAL, AdaBoost::DECISION_STUMP, 60);


	td1->read_data("er_classifier/training_data.txt");
	adb1.train_classifier(*td1, "er_classifier/adb1.classifier");
	
	for (int i = 0; i < td1->data.size(); i++)
	{
		if (adb1.predict(td1->data[i].fv) < 2.0)
		{
			td2->data.push_back(td1->data[i]);
		}
	}
	
	delete td1;

	td2->set_num(td2->data.size());
	td2->set_dim(td2->data.front().fv.size());
	adb2.train_classifier(*td2, "er_classifier/adb2.classifier");
}


void train_cascade()
{
	double Ftarget1 = 0.005;
	double f1 = 0.53;
	double d1 = 0.85;
	double Ftarget2 = 0.15;
	double f2 = 0.62;
	double d2 = 0.86;
	TrainingData *td1 = new TrainingData();
	TrainingData *tmp = new TrainingData();
	TrainingData *td2 = new TrainingData();
	AdaBoost *adb1 = new CascadeBoost(AdaBoost::REAL, AdaBoost::DECISION_STUMP, Ftarget1, f1, d1);
	AdaBoost *adb2 = new CascadeBoost(AdaBoost::REAL, AdaBoost::DECISION_STUMP, Ftarget2, f2, d2);

	freopen("er_classifier/log.txt", "w", stdout);

	chrono::high_resolution_clock::time_point start, middle, end;
	start = chrono::high_resolution_clock::now();

	cout << "Strong Text    Ftarget:" << Ftarget1 << " f=" << f1 << " d:" << d1 << endl;
	td1->read_data("er_classifier/training_data.txt");
	adb1->train_classifier(*td1, "er_classifier/cascade1.classifier");

	middle = chrono::high_resolution_clock::now();

	cout << endl << "Weak Text    Ftarget:" << Ftarget2 << " f=" << f2 << " d:" << d2 << endl;
	td2->read_data("er_classifier/training_data.txt");
	adb2->train_classifier(*td2, "er_classifier/cascade2.classifier");

	end = chrono::high_resolution_clock::now();

	cout << "strong training time:" << chrono::duration<double>(middle - start).count() * 1000 << " ms" << endl;
	cout << "weak training time:" << chrono::duration<double>(end - middle).count() * 1000 << " ms" << endl;
}


void bootstrap()
{
	ERFilter *erFilter = new ERFilter(THRESHOLD_STEP, MIN_ER_AREA, MAX_ER_AREA, NMS_STABILITY_T, NMS_OVERLAP_COEF);
	erFilter->stc = new CascadeBoost("er_classifier/strong.classifier");
	erFilter->wtc = new CascadeBoost("er_classifier/weak.classifier");


	int i = 0;
	for (int pic = 1; pic <= 10000; pic++)
	{
		char filename[100];
		//sprintf(filename, "res/neg/image_net_neg/%d.jpg", pic);
		sprintf(filename, "D:\\0.Projects\\image_data_set\\ICDAR2015\\Challenge1\\Challenge1_Test_Task3_Images\\word_%d.png", pic);
	

		ERs all, pool, strong, weak;
		Mat input = imread(filename, IMREAD_GRAYSCALE);
		if (input.empty()) continue;


		ER *root = erFilter->er_tree_extract(input);
		erFilter->non_maximum_supression(root, all, pool, input);
		erFilter->classify(pool, strong, weak, input);


		for (auto it : strong)
		{
			char imgname[30];
			sprintf(imgname, "res/tmp1/%d_%d.jpg", pic, i);
			imwrite(imgname, input(it->bound));
			i++;
		}

		for (auto it : weak)
		{
			char imgname[30];
			sprintf(imgname, "res/tmp1/%d_%d.jpg", pic, i);
			imwrite(imgname, input(it->bound));
			i++;
		}

		cout << pic << " finish " << endl;
	}
}

// Training Functions
void get_lbp_data()
{
	fstream fout = fstream("er_classifier/training_data.txt", fstream::out);

	ERFilter erFilter(THRESHOLD_STEP, MIN_ER_AREA, MAX_ER_AREA, NMS_STABILITY_T, NMS_OVERLAP_COEF);
	erFilter.ocr = new OCR();

	const int N = 2;
	const int normalize_size = 24;

	for (int i = 1; i <= 3; i++)
	{
		for (int pic = 0; pic <= 15000; pic++)
		{
			char filename[30];
			sprintf(filename, "res/neg/neg%d/%d.jpg", i, pic);

			Mat input = imread(filename, IMREAD_GRAYSCALE);
			if (input.empty())	continue;

			vector<double> spacial_hist = erFilter.make_LBP_hist(input, N, normalize_size);
			fout << -1;
			for (int f = 0; f < spacial_hist.size(); f++)
			{
				fout << " " << spacial_hist[f];
			}
			fout << endl;

			/*spacial_hist = erFilter.make_LBP_hist(255-input, N, normalize_size);
			fout << -1;
			for (int f = 0; f < spacial_hist.size(); f++)
				fout << " " << spacial_hist[f];
			fout << endl;*/


			cout << filename << " finish " << endl;
		}
	}
	


	for (int i = 1; i <= 1; i++)
	{
		for (int pic = 0; pic <= 15000; pic++)
		{
			char filename[30];
			sprintf(filename, "res/pos/pos%d/%d.jpg", i, pic);

			Mat input = imread(filename, IMREAD_GRAYSCALE);
			if (input.empty())	continue;

			vector<double> spacial_hist = erFilter.make_LBP_hist(input, N, normalize_size);
			fout << 1;
			for (int f = 0; f < spacial_hist.size(); f++)
			{
				fout << " " << spacial_hist[f];
			}
			fout << endl;

			spacial_hist = erFilter.make_LBP_hist(255 - input, N, normalize_size);
			fout << 1;
			for (int f = 0; f < spacial_hist.size(); f++)
			{
				fout << " " << spacial_hist[f];
			}
			fout << endl;

			cout << filename <<" finish " << endl;
		}
	}
}




void rotate_ocr_samples()
{
	const char *table = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz&()";	// 10 num, 52 alphabet, 3 symbol and 1 '\0'
	vector<string> font_name = { 
		"Arial", "Bitter", "Calibri", "Cambria", "Coda", "Comic_Sans_MS", "Courier_New", "Domine", "Droid_Serif", "Fine_Ming",
		"Gill_Sans", "Francois_One", "Georgia", "Impact", "Lato", "Neuton", "Open_Sans", "Oswald", "Oxygen", "Play", "PT_Serif", "Roboto_Slab", "Russo_One", 
		"Sans_Serif", "Syncopate", "Time_New_Roman", "Trebuchet_MS", "Twentieth_Century", "Ubuntu", "Verdana" };
	vector<string> font_type = { "Bold", "Bold_and_Italic", "Italic", "Normal" };
	vector<string> category = { "number", "upper", "lower", "symbol" };
	vector<int> cat_num = { 10,26,26,3 };

	OCR ocr = OCR();
	int n = 0;
	double rad = 0 / 180.0*CV_PI;
	for (int i = 0; i < font_name.size(); i++)
	{
		for (int j = 0; j < font_type.size(); j++)
		{
			int label = 0;
			for (int k = 0; k < category.size(); k++)
			{
				string path = String("ocr_classifier/" + font_name[i] + "/" + font_type[j] + "/" + category[k] + "/");
				for (int cat_it = 0; cat_it < cat_num[k]; cat_it++)
				{
					String filename = path + table[label] + ".jpg";
					label++;
					Mat img = imread(filename, IMREAD_GRAYSCALE);

					if (!img.empty())
						cout << filename << " done!" << endl;
					else
					{
						cout << filename << " not exist!" << endl;
						continue;
					}

					ocr.geometric_normalization(img, img, rad, false);

					char buf[256];
					sprintf(buf, "res/tmp2/%d.jpg",  n++);
					imwrite(buf, img);
				}
			}
		}
	}
}


void train_ocr_model()
{
	const char *table = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz&()";	// 10 num, 52 alphabet, 3 symbol and 1 '\0'
	vector<string> font_name = {
		"Arial", "Bitter", "Calibri", "Cambria", "Coda", "Comic_Sans_MS", "Courier_New", "Domine", "Droid_Serif", "Fine_Ming",
		"Gill_Sans", "Francois_One", "Georgia", "Impact", "Lato", "Neuton", "Open_Sans", "Oswald", "Oxygen", "Play", "PT_Serif", "Roboto_Slab", "Russo_One",
		"Sans_Serif", "Syncopate", "Time_New_Roman", "Trebuchet_MS", "Twentieth_Century", "Ubuntu", "Verdana" };
	vector<string> font_type = { "Bold", "Bold_and_Italic", "Italic", "Normal" };
	vector<string> category = {"number", "upper", "lower", "symbol" };
	vector<int> cat_num = { 10,26,26,3 };


	fstream fout = fstream("ocr_classifier/OCR.data", fstream::out);
	Mat feature;
	vector<int> labels;

	ERFilter erFilter(THRESHOLD_STEP, MIN_ER_AREA, MAX_ER_AREA, NMS_STABILITY_T, NMS_OVERLAP_COEF);
	erFilter.ocr = new OCR("ocr_classifier/OCR.model", OCR_IMG_L, OCR_FEATURE_L);

	for (int i = 0; i < font_name.size(); i++)
	{
		for (int j = 0; j < font_type.size(); j++)
		{
			int label = 0;
			for (int k = 0; k < category.size(); k++)
			{
				string path = String("ocr_classifier/" + font_name[i] + "/" + font_type[j] + "/" + category[k] + "/");
				for (int cat_it = 0; cat_it < cat_num[k]; cat_it++)
				{
					String filename = path + table[label]+ ".jpg";
					label++;

					Mat img = imread(filename, IMREAD_GRAYSCALE);

					if (!img.empty())
						cout << filename << " done!" << endl;
					else
					{
						cout << filename << " not exist!" << endl;
						continue;
					}

					fout << label-1;
					
					Mat ocr_img;
					threshold(255 - img, ocr_img, 200, 255, CV_THRESH_BINARY);
					erFilter.ocr->rotate_mat(ocr_img, ocr_img, 0, true);
					erFilter.ocr->ARAN(ocr_img, ocr_img, OCR_IMG_L);
					
					// get chain code svm node
					svm_node *fv = new svm_node[8 * OCR_FEATURE_L * OCR_FEATURE_L + 1];
					erFilter.ocr->extract_feature(ocr_img, fv);

					int m = 0;
					while (fv[m].index != -1)
					{
						fout << " " << fv[m].index << ":" << fv[m].value;
						m++;
					}
					fout << endl;
				}
			}
		}
	}

	// for Other
	int label = 0;
	for (int k = 0; k < category.size()-1; k++)
	{
		for (int cat_it = 0; cat_it < cat_num[k]; cat_it++)
		{
			string path = string("ocr_classifier/Other/" + category[k] + "/" + table[label] + "/");
			label++;
			for (int i = 0; i < 100; i++)
			{
				char buf[8];
				string filename = path + _itoa(i, buf, 10) + ".jpg";

				Mat img = imread(filename, IMREAD_GRAYSCALE);
				if (!img.empty())
					cout << filename << " done!" << endl;
				else
				{
					cout << filename << " not exist!" << endl;
					continue;
				}

				fout << label - 1;

				Mat ocr_img;
				threshold(255 - img, ocr_img, 200, 255, CV_THRESH_BINARY);
				erFilter.ocr->rotate_mat(ocr_img, ocr_img, 0, true);
				erFilter.ocr->ARAN(ocr_img, ocr_img, OCR_IMG_L);

				// get chain code svm node
				svm_node *fv = new svm_node[8 * OCR_FEATURE_L * OCR_FEATURE_L + 1];
				erFilter.ocr->extract_feature(ocr_img, fv);

				int m = 0;
				while (fv[m].index != -1)
				{
					fout << " " << fv[m].index << ":" << fv[m].value;
					m++;
				}
				fout << endl;
			}
		}
	}
	fout.close();

	// svm
	std::system("C:/libsvm-3.22/windows/svm-train.exe -b 1 -c 512.0.0 -g 0.0078125 ocr_classifier/OCR.data ocr_classifier/OCR.model");
	std::system("C:/libsvm-3.22/windows/svm-predict.exe -b 1 ocr_classifier/OCR.data ocr_classifier/OCR.model ocr_classifier/predict_result.txt");
}


void extract_ocr_sample()
{
	string path = "ocr_classifier/Calibri/Normal/";
	string file = path + "C.jpg";
	Mat input = imread(file, IMREAD_GRAYSCALE);
	
	ERFilter erFilter(1, 200, MAX_ER_AREA, NMS_STABILITY_T, NMS_OVERLAP_COEF);

	ER* root = erFilter.er_tree_extract(input);
	ERs all, pool, strong, weak;
	erFilter.non_maximum_supression(root, all, pool, input);
	int n = 0;
	for (auto it : pool)
	{
		char buf[100];
		sprintf(buf, "ocr_classifier/Calibri/Normal/%d.jpg", n++);
		imwrite(buf, input(it->bound));
	}

	for (auto it : pool)
	{
		rectangle(input, it->bound, Scalar(128));
	}

	imshow("input", input);
	waitKey(0);
}


void opencv_train()
{
	using namespace ml;
	Ptr<Boost> boost = Boost::create();
	Ptr<TrainData> trainData = TrainData::loadFromCSV("er_classifier/training_data.txt", 0, 0, 1, String(), ' ');
	boost->setBoostType(Boost::REAL);
	boost->setWeakCount(100);
	boost->setMaxDepth(1);
	boost->setWeightTrimRate(0);
	cout << "training..." << endl;
	boost->train(trainData);
	boost->save("er_classifier/opencv_classifier.xml");
}



// solve levenshtein distance(edit distance) by dynamic programming, 
// check https://vinayakgarg.wordpress.com/2012/12/10/edit-distance-using-dynamic-programming/ for more info
int levenshtein_distance(string str1, string str2)
{
#define INSERT_COST 1
#define DELETE_COST 1
#define REPLACE_COST 1

	// cost matrix
	// row -> str1 & col -> str2
	int size1 = str1.size();
	int size2 = str2.size();
	vector<vector<int>> cost(size1, vector<int>(size2));
	int i, j;

	// initialize the cost matrix
	for (i = 0; i<size1; i++) 
	{
		for (j = 0; j<size2; j++) 
		{
			if (i == 0) 
			{
				// source string is NULL
				// so we need 'j' insert operations
				cost[i][j] = j*INSERT_COST;
			}
			else if (j == 0) 
			{
				// target string is NULL
				// so we need 'i' delete operations
				cost[i][j] = i*DELETE_COST;
			}
			else 
			{
				cost[i][j] = -1;
			}
		}
	}
	
	//compute cost(i,j) and eventually return cost(m,n)
	for (i = 1; i<size1; i++) 
	{
		for (j = 1; j<size2; j++) 
		{
			int x = cost[i - 1][j] + DELETE_COST;
			int y = cost[i][j - 1] + INSERT_COST;
			// if str1(i-1) != str2(j-1), add the replace cost
			// we are comparing str1[i-1] and str2[j-1] since
			// the array index starts from 0
			int z = cost[i - 1][j - 1] + (str1[i - 1] != str2[j - 1])*REPLACE_COST;
			// as per our recursive formula
			cost[i][j] = min(x, min(y, z));
		}
	}

	// last cell of the matrix holds the answer
	return cost[size1 - 1][size2 - 1];
}