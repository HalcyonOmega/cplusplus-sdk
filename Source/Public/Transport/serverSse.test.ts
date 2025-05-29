import http from "http";
import { jest } from "@jest/globals";
import { SSEServerTransport } from "../Core/server/sse.js";

const createMockResponse = () => {
	const res = {
		writeHead: jest.fn<http.ServerResponse["writeHead"]>(),
		write: jest.fn<http.ServerResponse["write"]>().mockReturnValue(true),
		on: jest.fn<http.ServerResponse["on"]>(),
	};
	res.writeHead.mockReturnThis();
	res.on.mockReturnThis();

	return res as unknown as http.ServerResponse;
};

describe("SSEServerTransport", () => {
	describe("start method", () => {
		it("should correctly append SessionID to a simple relative endpoint", async () => {
			const mockRes = createMockResponse();
			const endpoint = "/messages";
			const transport = new SSEServerTransport(endpoint, mockRes);
			const expectedSessionID = transport.SessionID;

			await transport.start();

			expect(mockRes.writeHead).toHaveBeenCalledWith(200, expect.any(Object));
			expect(mockRes.write).toHaveBeenCalledTimes(1);
			expect(mockRes.write).toHaveBeenCalledWith(
				`event: endpoint\ndata: /messages?SessionID=${expectedSessionID}\n\n`,
			);
		});

		it("should correctly append SessionID to an endpoint with existing query parameters", async () => {
			const mockRes = createMockResponse();
			const endpoint = "/messages?foo=bar&baz=qux";
			const transport = new SSEServerTransport(endpoint, mockRes);
			const expectedSessionID = transport.SessionID;

			await transport.start();

			expect(mockRes.writeHead).toHaveBeenCalledWith(200, expect.any(Object));
			expect(mockRes.write).toHaveBeenCalledTimes(1);
			expect(mockRes.write).toHaveBeenCalledWith(
				`event: endpoint\ndata: /messages?foo=bar&baz=qux&SessionID=${expectedSessionID}\n\n`,
			);
		});

		it("should correctly append SessionID to an endpoint with a hash fragment", async () => {
			const mockRes = createMockResponse();
			const endpoint = "/messages#section1";
			const transport = new SSEServerTransport(endpoint, mockRes);
			const expectedSessionID = transport.SessionID;

			await transport.start();

			expect(mockRes.writeHead).toHaveBeenCalledWith(200, expect.any(Object));
			expect(mockRes.write).toHaveBeenCalledTimes(1);
			expect(mockRes.write).toHaveBeenCalledWith(
				`event: endpoint\ndata: /messages?SessionID=${expectedSessionID}#section1\n\n`,
			);
		});

		it("should correctly append SessionID to an endpoint with query parameters and a hash fragment", async () => {
			const mockRes = createMockResponse();
			const endpoint = "/messages?key=value#section2";
			const transport = new SSEServerTransport(endpoint, mockRes);
			const expectedSessionID = transport.SessionID;

			await transport.start();

			expect(mockRes.writeHead).toHaveBeenCalledWith(200, expect.any(Object));
			expect(mockRes.write).toHaveBeenCalledTimes(1);
			expect(mockRes.write).toHaveBeenCalledWith(
				`event: endpoint\ndata: /messages?key=value&SessionID=${expectedSessionID}#section2\n\n`,
			);
		});

		it('should correctly handle the root path endpoint "/"', async () => {
			const mockRes = createMockResponse();
			const endpoint = "/";
			const transport = new SSEServerTransport(endpoint, mockRes);
			const expectedSessionID = transport.SessionID;

			await transport.start();

			expect(mockRes.writeHead).toHaveBeenCalledWith(200, expect.any(Object));
			expect(mockRes.write).toHaveBeenCalledTimes(1);
			expect(mockRes.write).toHaveBeenCalledWith(
				`event: endpoint\ndata: /?SessionID=${expectedSessionID}\n\n`,
			);
		});

		it('should correctly handle an empty string endpoint ""', async () => {
			const mockRes = createMockResponse();
			const endpoint = "";
			const transport = new SSEServerTransport(endpoint, mockRes);
			const expectedSessionID = transport.SessionID;

			await transport.start();

			expect(mockRes.writeHead).toHaveBeenCalledWith(200, expect.any(Object));
			expect(mockRes.write).toHaveBeenCalledTimes(1);
			expect(mockRes.write).toHaveBeenCalledWith(
				`event: endpoint\ndata: /?SessionID=${expectedSessionID}\n\n`,
			);
		});
	});
});
