// Code generated by mockery v0.0.0-dev. DO NOT EDIT.

package mocks

import (
	mock "github.com/stretchr/testify/mock"
	interfaces "go.skia.org/skia/bazel/exporter/interfaces"
)

// FileSystem is an autogenerated mock type for the FileSystem type
type FileSystem struct {
	mock.Mock
}

// OpenFile provides a mock function with given fields: path
func (_m *FileSystem) OpenFile(path string) (interfaces.Writer, error) {
	ret := _m.Called(path)

	if len(ret) == 0 {
		panic("no return value specified for OpenFile")
	}

	var r0 interfaces.Writer
	var r1 error
	if rf, ok := ret.Get(0).(func(string) (interfaces.Writer, error)); ok {
		return rf(path)
	}
	if rf, ok := ret.Get(0).(func(string) interfaces.Writer); ok {
		r0 = rf(path)
	} else {
		if ret.Get(0) != nil {
			r0 = ret.Get(0).(interfaces.Writer)
		}
	}

	if rf, ok := ret.Get(1).(func(string) error); ok {
		r1 = rf(path)
	} else {
		r1 = ret.Error(1)
	}

	return r0, r1
}

// ReadFile provides a mock function with given fields: filename
func (_m *FileSystem) ReadFile(filename string) ([]byte, error) {
	ret := _m.Called(filename)

	if len(ret) == 0 {
		panic("no return value specified for ReadFile")
	}

	var r0 []byte
	var r1 error
	if rf, ok := ret.Get(0).(func(string) ([]byte, error)); ok {
		return rf(filename)
	}
	if rf, ok := ret.Get(0).(func(string) []byte); ok {
		r0 = rf(filename)
	} else {
		if ret.Get(0) != nil {
			r0 = ret.Get(0).([]byte)
		}
	}

	if rf, ok := ret.Get(1).(func(string) error); ok {
		r1 = rf(filename)
	} else {
		r1 = ret.Error(1)
	}

	return r0, r1
}

// NewFileSystem creates a new instance of FileSystem. It also registers a testing interface on the mock and a cleanup function to assert the mocks expectations.
// The first argument is typically a *testing.T value.
func NewFileSystem(t interface {
	mock.TestingT
	Cleanup(func())
}) *FileSystem {
	mock := &FileSystem{}
	mock.Mock.Test(t)

	t.Cleanup(func() { mock.AssertExpectations(t) })

	return mock
}