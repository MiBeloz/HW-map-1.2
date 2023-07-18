#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <execution>
#include <condition_variable>
#include <random>

using namespace std::chrono_literals;

std::once_flag flag;


unsigned long long sum_two_vectors(const std::vector<int>& vector_1, const std::vector<int>& vector_2);
double time_for_sum_two_vectors(const std::vector<int>& vector_1, const std::vector<int>& vector_2, const int thread_count);
void print_hardware_concurrency();

int main() {
	setlocale(LC_ALL, "ru");
	std::cout << "\tПараллельные вычисления\n\n" << std::endl;

    std::vector<std::vector<int>> vectors;
    std::vector<double> fast_time;

    //создание векторов
    const int threads_count = 16;
    const int start_elements = 1000;
    const int vectors_count = 4;
    const int step_elements = 10;
    for (int i = 0, j = start_elements; i < vectors_count; ++i, j *= step_elements) {
        vectors.emplace_back(std::vector<int>(j));
        std::mt19937 gen;
        std::uniform_int_distribution<int> dist(0, j);
        std::generate(std::execution::par, vectors[i].begin(), vectors[i].end(), [&gen, &dist]() {return dist(gen); });

        //vectors.emplace_back(std::vector<int>(j, 1));
    }
    
    //расчет и вывод
    for (int i = 1; i <= threads_count; i *= 2) {
        if (i == 1) {
            for (size_t j = 0; j < vectors.size(); ++j) {
                fast_time.emplace_back(time_for_sum_two_vectors(vectors[j], vectors[j], i));
            }

            std::cout << i << " потоков:\t";
            for (size_t j = 0; j < fast_time.size(); ++j) {
                std::cout << fast_time[j] << " сек." << "\t\t";
            }
            std::cout << std::endl << std::endl;
        }
        else {
            double time = 0;
            std::cout << i << " потоков:\t";
            for (size_t j = 0; j < vectors.size(); ++j) {
                time = time_for_sum_two_vectors(vectors[j], vectors[j], i);
                std::cout << time << " сек." << "\t\t";
                if (time < fast_time[j]) { fast_time[j] = time; }
            }
            std::cout << std::endl << std::endl;
        }
    }

    std::cout << "\nСамый быстрый результат для:" << std::endl;
    for (size_t i = 0; i < fast_time.size(); ++i) {
        std::cout << "\t" << vectors[i].size() << " элементов: \t" << fast_time[i] << std::endl;
    }

	system("pause > nul");
	return 0;
}

unsigned long long sum_two_vectors(const std::vector<int>& vector_1, const std::vector<int>& vector_2) {
    unsigned long long result = 0;

    if (vector_1.size() == vector_2.size()) {
        for (size_t i = 0; i < vector_2.size(); ++i) {
            result += vector_1[i] + vector_2[i];
        }
    }
    else if (vector_1.size() > vector_2.size()) {
        for (size_t i = 0; i < vector_2.size(); ++i) {
            result += vector_1[i] + vector_2[i];
        }
        for (size_t i = vector_2.size(); i < vector_1.size(); ++i) {
            result += vector_1[i];
        }
    }
    else {
        for (size_t i = 0; i < vector_1.size(); ++i) {
            result += vector_1[i] + vector_2[i];
        }
        for (size_t i = vector_1.size(); i < vector_2.size(); ++i) {
            result += vector_2[i];
        }
    }

    return result;
}

double time_for_sum_two_vectors(const std::vector<int>& vector_1, const std::vector<int>& vector_2, const int thread_count) {
    std::call_once(flag, print_hardware_concurrency);

    unsigned long long sum_vector = 0;
    std::vector<std::vector<int>> vect_v;
    std::vector<unsigned long long> threads_sum;
    std::vector<std::thread> vect_t;

    //если векторы одинаковой длины:
    if (!(vector_1.size() % thread_count) && !(vector_2.size() % thread_count)) {
        int step_1 = static_cast<int>(vector_1.size()) / thread_count;
        int step_2 = static_cast<int>(vector_2.size()) / thread_count;
        for (int i = 0; i < thread_count; ++i) {
            vect_v.emplace_back(vector_1.begin() + i * step_1, vector_1.begin() + (i + 1) * step_1);
            vect_v.emplace_back(vector_2.begin() + i * step_2, vector_2.begin() + (i + 1) * step_2);
        }
    }
    //иначе, если разной длины:
    else {
        int k1 = 0, k2 = 0;
        while ((vector_1.size() - k1) % thread_count) {
            k1++;
        }
        int step_1 = (static_cast<int>(vector_1.size()) - k1) / thread_count;
        while ((vector_2.size() - k2) % thread_count) {
            k2++;
        }
        int step_2 = (static_cast<int>(vector_2.size()) - k2) / thread_count;
        for (int i = 0; i < thread_count; ++i) {
            if (i == thread_count - 1) {
                vect_v.emplace_back(vector_1.begin() + i * step_1, vector_1.begin() + (i + 1) * step_1 + k1);
                vect_v.emplace_back(vector_2.begin() + i * step_2, vector_2.begin() + (i + 1) * step_2 + k2);
            }
            else {
                vect_v.emplace_back(vector_1.begin() + i * step_1, vector_1.begin() + (i + 1) * step_1);
                vect_v.emplace_back(vector_2.begin() + i * step_2, vector_2.begin() + (i + 1) * step_2);
            }
        }
    }

    //вычисление суммы пар векторов, в зависимости от количества потоков
    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0, j = 0; i < thread_count; ++i, j += 2) {
        std::thread t([j, thread_count, &vect_v, &threads_sum]
            {
                threads_sum.emplace_back(sum_two_vectors(vect_v[j], vect_v[j + 1]));
            });
        vect_t.emplace_back(std::move(t));
    }
    for (std::thread& t : vect_t)
    {
        if (t.joinable())
            t.join();
    }

    //общая сумма
    for (const auto& it : threads_sum) {
        sum_vector += it;
    }
    auto end = std::chrono::steady_clock::now();

    //вывод суммы(для проверок)
    //std::cout << sum_vector << " ";

    std::chrono::duration<double> dur = end - start;
    return dur.count();
}

void print_hardware_concurrency() {
    std::cout << "Количество аппаратных ядер - " << std::thread::hardware_concurrency() << std::endl << std::endl;
    std::cout << "\t\t1000\t\t\t10000\t\t\t100000\t\t\t1000000" << std::endl << std::endl;
}
