#include <iostream>
#include <set>
#include <functional>
#include <chrono>
#include <random>
#include <boost/icl/separate_interval_set.hpp>

using namespace std;

struct time_interval {
	bool overlaps(time_interval t) const { return t.begin < end && begin < t.end; }

	bool is_empty() const { return begin >= end; }
	
	int begin {};
	int end {};
};

using Map = set<time_interval>;

bool operator==(time_interval l, time_interval r) {
	return l.begin == r.begin && l.end == r.end;
}

bool operator!=(time_interval l, time_interval r) {
	return !(l == r);
}

ostream& operator<<(ostream& os, time_interval t) {
	return os << "[" << t.begin << ", " << t.end << ")";
}


ostream& operator<<(ostream& os, const Map& m) {
	for (auto [interval,value] : m) {
		os << interval << ": " << value  << '\n';
	}
	return os;
}

time_interval hull(time_interval l, time_interval r) {
	return {min(l.begin, r.begin), max(l.end, r.end)};
}

bool operator<(time_interval l, time_interval r) {
	if (l.begin == r.begin) return l.end < r.end;
	return l.begin < r.begin;
}

void push_and_merge(Map& m, time_interval t) {
	// cout << m;

	time_interval h {t};

	auto pos {m.end()};

	{
		auto i {m.upper_bound(t)};
		for (; i != m.end() && h.overlaps(*i);) {
			h = hull(h, *i);
			i = m.erase(i);
		}

		pos = i;
	}

	{
		auto i {make_reverse_iterator(pos)};
		for (; i != m.rend() && h.overlaps(*i);) {
			h = hull(h, *i);
			i = make_reverse_iterator(m.erase(prev(i.base())));
		}

		if (i != m.rend()) pos = prev(i.base());
	}

	m.insert(pos, h);
}

int main() {
	using IntervalSet = boost::icl::separate_interval_set<int>;
	using Interval = IntervalSet::interval_type;

	IntervalSet set;
	Map m;

	constexpr pair limits {1, 100};
	constexpr int max {10'000'000};
	constexpr int offset_start {0};
	constexpr int stride {0};

	auto seed {random_device{}()};

	{
		mt19937 eng {seed};
		auto d {uniform_int_distribution{limits.first, limits.second}};
		auto gen {bind(d, eng)};

		int off {offset_start};

		auto t1 {chrono::system_clock::now()};

		for (int i{}; i != max; ++i) {
			time_interval t {gen() + off, gen() + off};
			if (t.is_empty()) continue;
			if (t.begin > t.end) swap(t.begin, t.end);
			// cout << t << '\n';
			push_and_merge(m, t);
			off += stride;
		}

		auto t2 {chrono::system_clock::now()};
		cout << "MyMap Duration: " << chrono::duration_cast<chrono::milliseconds>(t2-t1).count() << " ms\n";
	}

	{
		mt19937 eng {seed};
		auto d {uniform_int_distribution{limits.first, limits.second}};
		auto gen {bind(d, eng)};

		int off {offset_start};

		auto t1 {chrono::system_clock::now()};

		for (int i{}; i != max; ++i) {
			time_interval t {gen() + off, gen() + off};
			if (t.is_empty()) continue;
			if (t.begin > t.end) swap(t.begin, t.end);
			// cout << t << '\n';
			set.insert(Interval::right_open(t.begin, t.end));
			off += stride;
		}

		auto t2 {chrono::system_clock::now()};
		cout << "Boost Duration: " << chrono::duration_cast<chrono::milliseconds>(t2-t1).count() << " ms\n";
	}

	for (auto [i,j] {pair{set.begin(), m.begin()}}; i != set.end() && j != m.end(); ++i, ++j) {
		if (i->lower() != j->begin || i->upper() != j->end) throw runtime_error{"Do not match!"};
	}

	cout << "MyMap size: " << m.size() << '\n';
	cout << "Boost size: " << set.iterative_size() << '\n';
}

